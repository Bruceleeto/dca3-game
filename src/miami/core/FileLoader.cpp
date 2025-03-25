#include "common.h"
#include <ctype.h>
#include "main.h"

#include "General.h"
#include "Quaternion.h"
#include "ModelInfo.h"
#include "ModelIndices.h"
#include "TempColModels.h"
#include "VisibilityPlugins.h"
#include "FileMgr.h"
#include "HandlingMgr.h"
#include "CarCtrl.h"
#include "PedType.h"
#include "AnimManager.h"
#include "Game.h"
#include "RwHelper.h"
#include "NodeName.h"
#include "TxdStore.h"
#include "PathFind.h"
#include "ObjectData.h"
#include "DummyObject.h"
#include "World.h"
#include "Zones.h"
#include "ZoneCull.h"
#include "CdStream.h"
#include "FileLoader.h"
#include "MemoryHeap.h"
#include "Streaming.h"
#include "ColStore.h"
#include "Occlusion.h"

void* re3StreamingAlloc(size_t size);

char CFileLoader::ms_line[256];

const char*
GetFilename(const char *filename)
{
	char *s = strrchr((char*)filename, '\\');
	return s ? s+1 : filename;
}

void
LoadingScreenLoadingFile(const char *filename)
{
	sprintf(gString, "Loading %s", GetFilename(filename));
	LoadingScreen("Loading the Game", gString, nil);
}

void
CFileLoader::LoadLevel(const char *filename)
{
	int fd;
	RwTexDictionary *savedTxd;
	bool objectsLoaded;
	char *line;
	char txdname[64];

	savedTxd = RwTexDictionaryGetCurrent();
	objectsLoaded = false;
	if(savedTxd == nil){
		savedTxd = RwTexDictionaryCreate();
		RwTexDictionarySetCurrent(savedTxd);
	}
	fd = CFileMgr::OpenFile(filename, "r");
	assert(fd > 0);

	for(line = LoadLine(fd); line; line = LoadLine(fd)){
		if(*line == '#')
			continue;

		if(strncmp(line, "EXIT", 4) == 0)
			break;

		if(strncmp(line, "IMAGEPATH", 9) == 0){
			RwImageSetPath(line + 10);
		}else if(strncmp(line, "TEXDICTION", 10) == 0){
			PUSH_MEMID(MEMID_TEXTURES);
			strcpy(txdname, line+11);
			LoadingScreenLoadingFile(txdname);
			RwTexDictionary *txd = LoadTexDictionary(txdname);
			AddTexDictionaries(savedTxd, txd);
			RwTexDictionaryDestroy(txd);
			POP_MEMID();
		}else if(strncmp(line, "COLFILE", 7) == 0){
			LoadingScreenLoadingFile(line+10);
			LoadCollisionFile(line+10, 0);
		}else if(strncmp(line, "MODELFILE", 9) == 0){
			LoadingScreenLoadingFile(line + 10);
			LoadModelFile(line + 10);
		}else if(strncmp(line, "HIERFILE", 8) == 0){
			LoadingScreenLoadingFile(line + 9);
			LoadClumpFile(line + 9);
		}else if(strncmp(line, "IDE", 3) == 0){
			LoadingScreenLoadingFile(line + 4);
			LoadObjectTypes(line + 4);
		}else if(strncmp(line, "IPL", 3) == 0){
			if(!objectsLoaded){
				LoadingScreenLoadingFile("Collision");
				PUSH_MEMID(MEMID_WORLD);
				CObjectData::Initialise("DATA\\OBJECT.DAT");
				CStreaming::Init();
				POP_MEMID();
				PUSH_MEMID(MEMID_COLLISION);
				CColStore::LoadAllCollision();
				POP_MEMID();
				for(int i = 0; i < MODELINFOSIZE; i++)
					if(CModelInfo::GetModelInfo(i))
						CModelInfo::GetModelInfo(i)->ConvertAnimFileIndex();
				objectsLoaded = true;
			}
			PUSH_MEMID(MEMID_WORLD);
			LoadingScreenLoadingFile(line + 4);
			LoadScene(line + 4);
			POP_MEMID();
		}else if(strncmp(line, "SPLASH", 6) == 0){
#ifndef DISABLE_LOADING_SCREEN
			LoadSplash(GetRandomSplashScreen());
#endif
#ifndef GTA_PS2
		}else if(strncmp(line, "CDIMAGE", 7) == 0){
			CdStreamAddImage(line + 8);
#endif
		}
	}

	CFileMgr::CloseFile(fd);
	RwTexDictionarySetCurrent(savedTxd);

	int i;
	for(i = 1; i < COLSTORESIZE; i++)
		if(CColStore::GetSlot(i))
			CColStore::GetBoundingBox(i).Grow(120.0f);
	CWorld::RepositionCertainDynamicObjects();
	CColStore::RemoveAllCollision();
}

char*
CFileLoader::LoadLine(int fd)
{
	int i;
	char *line;

	if(CFileMgr::ReadLine(fd, ms_line, 256) == false)
		return nil;
	for(i = 0; ms_line[i] != '\0'; i++)
		if(ms_line[i] < ' ' || ms_line[i] == ',')
			ms_line[i] = ' ';
	for(line = ms_line; *line <= ' ' && *line != '\0'; line++);
	return line;
}

RwTexDictionary*
CFileLoader::LoadTexDictionary(const char *filename)
{
	RwTexDictionary *txd;
	RwStream *stream;

	txd = nil;
	stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, filename);
	debug("Loading texture dictionary file %s\n", filename);
	if(stream){
		if(RwStreamFindChunk(stream, rwID_TEXDICTIONARY, nil, nil))
			txd = RwTexDictionaryGtaStreamRead(stream);
		RwStreamClose(stream, nil);
	}
	if(txd == nil)
		txd = RwTexDictionaryCreate();
	return txd;
}

struct ColHeader
{
	uint32 ident;
	uint32 size;
};

void
CFileLoader::LoadCollisionFile(const char *filename, uint8 colSlot)
{
	int fd;
	char modelname[24];
	CBaseModelInfo *mi;
	ColHeader header;

	PUSH_MEMID(MEMID_COLLISION);

	debug("Loading collision file %s\n", filename);
	fd = CFileMgr::OpenFile(filename, "rb");
	assert(fd > 0);

	while(CFileMgr::Read(fd, (char*)&header, sizeof(header))){
		assert(header.ident == 'CLOC');
		CFileMgr::Read(fd, (char*)work_buff, header.size);
		memcpy(modelname, work_buff, 24);

		mi = CModelInfo::GetModelInfo(modelname, nil);
		if(mi){
			if(mi->GetColModel() && mi->DoesOwnColModel()){
				LoadCollisionModel(work_buff+24, *mi->GetColModel(), modelname);
			}else{
				CColModel *model = new CColModel;
				model->level = colSlot;
				LoadCollisionModel(work_buff+24, *model, modelname);
				mi->SetColModel(model, true);
			}
		}else{
			debug("colmodel %s can't find a modelinfo\n", modelname);
		}
	}

	CFileMgr::CloseFile(fd);

	POP_MEMID();
}


bool
CFileLoader::LoadCollisionFileFirstTime(uint8 *buffer, uint32 size, uint8 colSlot)
{
	uint32 modelsize;
	char modelname[24];
	CBaseModelInfo *mi;
	ColHeader *header;
	int32 modelIndex;

	while(size > 8){
		header = (ColHeader*)buffer;
		modelsize = header->size;
		if(header->ident != 'CLOC')
			return size-8 < CDSTREAM_SECTOR_SIZE;
		memcpy(modelname, buffer+8, 24);
		memcpy(work_buff, buffer+32, modelsize-24);
		size -= 32 + (modelsize-24);
		buffer += 32 + (modelsize-24);
		if(modelsize > 15*1024)
			debug("colmodel %s is huge, size %d\n", modelname, modelsize);

		mi = CModelInfo::GetModelInfo(modelname, &modelIndex);
		if(mi){
			CColStore::IncludeModelIndex(colSlot, modelIndex);
			CColModel *model = new CColModel;
			model->level = colSlot;
			LoadCollisionModel(work_buff, *model, modelname);
			mi->SetColModel(model, true);
		}else{
			debug("colmodel %s can't find a modelinfo\n", modelname);
		}
	}
	return true;
}

bool
CFileLoader::LoadCollisionFile(uint8 *buffer, uint32 size, uint8 colSlot)
{
	uint32 modelsize;
	char modelname[24];
	CBaseModelInfo *mi;
	ColHeader *header;

	while(size > 8){
		header = (ColHeader*)buffer;
		modelsize = header->size;
		if(header->ident != 'CLOC')
			return size-8 < CDSTREAM_SECTOR_SIZE;
		memcpy(modelname, buffer+8, 24);
		memcpy(work_buff, buffer+32, modelsize-24);
		size -= 32 + (modelsize-24);
		buffer += 32 + (modelsize-24);
		if(modelsize > 15*1024)
			debug("colmodel %s is huge, size %d\n", modelname, modelsize);

		mi = CModelInfo::GetModelInfo(modelname, CColStore::GetSlot(colSlot)->minIndex, CColStore::GetSlot(colSlot)->maxIndex);
		if(mi){
			if(mi->GetColModel()){
				LoadCollisionModel(work_buff, *mi->GetColModel(), modelname);
			}else{
				CColModel *model = new CColModel;
				model->level = colSlot;
				LoadCollisionModel(work_buff, *model, modelname);
				mi->SetColModel(model, true);
			}
		}else{
			debug("colmodel %s can't find a modelinfo\n", modelname);
		}
	}
	return true;
}

void
CFileLoader::LoadCollisionModel(uint8 *buf, CColModel &model, char *modelname)
{
	int i;

	model.boundingSphere.radius = *(float*)(buf);
	model.boundingSphere.center.x = *(float*)(buf+4);
	model.boundingSphere.center.y = *(float*)(buf+8);
	model.boundingSphere.center.z = *(float*)(buf+12);
	model.boundingBox.min.x = *(float*)(buf+16);
	model.boundingBox.min.y = *(float*)(buf+20);
	model.boundingBox.min.z = *(float*)(buf+24);
	model.boundingBox.max.x = *(float*)(buf+28);
	model.boundingBox.max.y = *(float*)(buf+32);
	model.boundingBox.max.z = *(float*)(buf+36);
	model.numSpheres = *(int16*)(buf+40);
	buf += 44;
	if (model.spheres) {
		RwFree(model.spheres);
	}
	if (model.lines) {
		RwFree(model.lines);
	}
	if (model.boxes) {
		RwFree(model.boxes);
	}
	if (model.vertices) {
		RwFree(model.vertices);
	}
	if (model.triangles) {
		RwFree(model.triangles);
	}
	if (model.trianglePlanes) {
		CCollision::RemoveTrianglePlanes(&model);
	}
	if(model.numSpheres > 0){
		model.spheres = (CColSphere*)RwMalloc(model.numSpheres*sizeof(CColSphere));
		REGISTER_MEMPTR(&model.spheres);
		for(i = 0; i < model.numSpheres; i++){
			model.spheres[i].Set(*(float*)buf, *(CVector*)(buf+4), buf[16], buf[17]);
			buf += 20;
		}
	}else
		model.spheres = nil;

	model.numLines = *(int16*)buf;
	buf += 4;
	if(model.numLines > 0){
		//model.lines = (CColLine*)RwMalloc(model.numLines*sizeof(CColLine));
		REGISTER_MEMPTR(&model.lines);
		for(i = 0; i < model.numLines; i++){
			//model.lines[i].Set(*(CVector*)buf, *(CVector*)(buf+12));
			buf += 24;
		}
	}else
		model.lines = nil;
	model.numLines = 0;
	model.lines = nil;

	model.numBoxes = *(int16*)buf;
	buf += 4;
	if(model.numBoxes > 0){
		model.boxes = (CColBox*)RwMalloc(model.numBoxes*sizeof(CColBox));
		REGISTER_MEMPTR(&model.boxes);
		for(i = 0; i < model.numBoxes; i++){
			model.boxes[i].Set(*(CVector*)buf, *(CVector*)(buf+12), buf[24], buf[25]);
			buf += 28;
		}
	}else
		model.boxes = nil;

	int32 numVertices = *(int16*)buf;
	buf += 4;
	if(numVertices > 0){
		model.vertices = (CompressedVector*)RwMalloc(numVertices*sizeof(CompressedVector));
		REGISTER_MEMPTR(&model.vertices);
		for(i = 0; i < numVertices; i++){
			model.vertices[i].SetFixed(*(int16*)buf, *(int16*)(buf+2), *(int16*)(buf+4));
#if 0
			if(Abs(*(float*)buf) >= 256.0f ||
			   Abs(*(float*)(buf+4)) >= 256.0f ||
			   Abs(*(float*)(buf+8)) >= 256.0f)
				printf("%s:Collision volume too big\n", modelname);
#endif
			buf += 6;
		}
	}else
		model.vertices = nil;

	model.numTriangles = *(int16*)buf;
	buf += 4;
	if(model.numTriangles > 0){
		model.triangles = (CColTriangle*)re3StreamingAlloc(model.numTriangles*sizeof(CColTriangle));
		REGISTER_MEMPTR(&model.triangles);
		for(i = 0; i < model.numTriangles; i++){
			model.triangles[i].Set(*(uint16*)buf, *(uint16*)(buf+2), *(uint16*)(buf+4), buf[6]);
			buf += 8;
			assert(model.triangles[i].a < numVertices);
			assert(model.triangles[i].b < numVertices);
			assert(model.triangles[i].c < numVertices);
		}
	}else
		model.triangles = nil;
}

static void
GetNameAndLOD(char *nodename, char *name, int *n)
{
	char *underscore = nil;
	for(char *s = nodename; *s != '\0'; s++){
		if(s[0] == '_' && (s[1] == 'l' || s[1] == 'L') && isdigit(s[2]))
			underscore = s;
	}
	if(underscore){
		strncpy(name, nodename, underscore - nodename);
		name[underscore - nodename] = '\0';
		*n = atoi(underscore + 2);
	}else{
		strncpy(name, nodename, 24);
		*n = 0;
	}
}

RpAtomic*
CFileLoader::FindRelatedModelInfoCB(RpAtomic *atomic, void *data)
{
	CSimpleModelInfo *mi;
	char *nodename, name[24];
	int n;
	RpClump *clump = (RpClump*)data;

	nodename = GetFrameNodeName(RpAtomicGetFrame(atomic));
	GetNameAndLOD(nodename, name, &n);
	mi = (CSimpleModelInfo*)CModelInfo::GetModelInfo(name, nil);
	if(mi){
		assert(mi->IsSimple());
		CVisibilityPlugins::SetAtomicRenderCallback(atomic, nil);
		mi->SetAtomic(n, atomic);
		RpClumpRemoveAtomic(clump, atomic);
		RpAtomicSetFrame(atomic, RwFrameCreate());
		CVisibilityPlugins::SetAtomicModelInfo(atomic, mi);
	}else{
		debug("Can't find Atomic %s\n", name);
	}

	return atomic;
}

#ifdef LIBRW
void
InitClump(RpClump *clump)
{
	RpClumpForAllAtomics(clump, ConvertPlatformAtomic, nil);
}
#else
#define InitClump(clump)
#endif

void
CFileLoader::LoadModelFile(const char *filename)
{
	RwStream *stream;
	RpClump *clump;

	debug("Loading model file %s\n", filename);
	stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, filename);
	if(RwStreamFindChunk(stream, rwID_CLUMP, nil, nil)){
		clump = RpClumpStreamRead(stream);
		if(clump){
			InitClump(clump);
			RpClumpForAllAtomics(clump, FindRelatedModelInfoCB, clump);
			RpClumpDestroy(clump);
		}
	}
	RwStreamClose(stream, nil);
}

void
CFileLoader::LoadClumpFile(const char *filename)
{
	RwStream *stream;
	RpClump *clump;
	char *nodename, name[24];
	int n;
	CClumpModelInfo *mi;

	debug("Loading model file %s\n", filename);
	stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, filename);
	while(RwStreamFindChunk(stream, rwID_CLUMP, nil, nil)){
		clump = RpClumpStreamRead(stream);
		if(clump){
			nodename = GetFrameNodeName(RpClumpGetFrame(clump));
			GetNameAndLOD(nodename, name, &n);
			mi = (CClumpModelInfo*)CModelInfo::GetModelInfo(name, nil);
			if(mi){
				InitClump(clump);
				assert(mi->IsClump());
				mi->SetClump(clump);
			}else
				RpClumpDestroy(clump);
		}
	}
	RwStreamClose(stream, nil);
}

bool
CFileLoader::LoadClumpFile(RwStream *stream, uint32 id)
{
	RpClump *clump;
	CClumpModelInfo *mi;

	if(!RwStreamFindChunk(stream, rwID_CLUMP, nil, nil))
		return false;
	clump = RpClumpStreamRead(stream);
	if(clump == nil)
		return false;
	InitClump(clump);
	mi = (CClumpModelInfo*)CModelInfo::GetModelInfo(id);
	mi->SetClump(clump);
	return true;
}

bool
CFileLoader::StartLoadClumpFile(RwStream *stream, uint32 id)
{
	if(RwStreamFindChunk(stream, rwID_CLUMP, nil, nil)){
		printf("Start loading %s\n", CModelInfo::GetModelInfo(id)->GetModelName());
		return RpClumpGtaStreamRead1(stream);
	}else{
		printf("FAILED\n");
		return false;
	}
}

bool
CFileLoader::FinishLoadClumpFile(RwStream *stream, uint32 id)
{
	RpClump *clump;
	CClumpModelInfo *mi;

	printf("Finish loading %s\n", CModelInfo::GetModelInfo(id)->GetModelName());
	clump = RpClumpGtaStreamRead2(stream);

	if(clump){
		InitClump(clump);
		mi = (CClumpModelInfo*)CModelInfo::GetModelInfo(id);
		mi->SetClump(clump);
		return true;
	}else{
		printf("FAILED\n");
		return false;
	}
}

CSimpleModelInfo *gpRelatedModelInfo;

bool
CFileLoader::LoadAtomicFile(RwStream *stream, uint32 id)
{
	RpClump *clump;

	if(RwStreamFindChunk(stream, rwID_CLUMP, nil, nil)){
		clump = RpClumpStreamRead(stream);
		if(clump == nil)
			return false;
		InitClump(clump);
		gpRelatedModelInfo = (CSimpleModelInfo*)CModelInfo::GetModelInfo(id);
		RpClumpForAllAtomics(clump, SetRelatedModelInfoCB, clump);
		RpClumpDestroy(clump);
	}
	return true;
}

RpAtomic*
CFileLoader::SetRelatedModelInfoCB(RpAtomic *atomic, void *data)
{
	char *nodename, name[24];
	int n;
	RpClump *clump = (RpClump*)data;

	nodename = GetFrameNodeName(RpAtomicGetFrame(atomic));
	GetNameAndLOD(nodename, name, &n);
	CVisibilityPlugins::SetAtomicRenderCallback(atomic, nil);
	gpRelatedModelInfo->SetAtomic(n, atomic);
	RpClumpRemoveAtomic(clump, atomic);
	RpAtomicSetFrame(atomic, RwFrameCreate());
	CVisibilityPlugins::SetAtomicModelInfo(atomic, gpRelatedModelInfo);
	return atomic;
}

RpClump*
CFileLoader::LoadAtomicFile2Return(const char *filename)
{
	RwStream *stream;
	RpClump *clump;

	clump = nil;
	debug("Loading model file %s\n", filename);
	stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, filename);
	if(RwStreamFindChunk(stream, rwID_CLUMP, nil, nil))
		clump = RpClumpStreamRead(stream);
	if(clump)
		InitClump(clump);
	RwStreamClose(stream, nil);
	return clump;
}

static RwTexture*
MoveTexturesCB(RwTexture *texture, void *pData)
{
	RwTexDictionaryAddTexture((RwTexDictionary*)pData, texture);
	return texture;
}

void
CFileLoader::AddTexDictionaries(RwTexDictionary *dst, RwTexDictionary *src)
{
	RwTexDictionaryForAllTextures(src, MoveTexturesCB, dst);
}

#define isLine3(l, a, b, c) ((l[0] == a) && (l[1] == b) && (l[2] == c))
#define isLine4(l, a, b, c, d) ((l[0] == a) && (l[1] == b) && (l[2] == c) && (l[3] == d))

void
CFileLoader::LoadObjectTypes(const char *filename)
{
	enum {
		NONE,
		OBJS,
		MLO,	// unused but enum still has it
		TOBJ,
		WEAP,
		HIER,
		CARS,
		PEDS,
		PATH,
		TWODFX
	};
	char *line;
	int fd;
	int section;
	int pathIndex;
	int id, pathType;
	int minID, maxID;

	section = NONE;
	minID = INT32_MAX;
	maxID = -1;
	pathIndex = -1;
	debug("Loading object types from %s...\n", filename);

	fd = CFileMgr::OpenFile(filename, "rb");
	assert(fd > 0);
	for(line = CFileLoader::LoadLine(fd); line; line = CFileLoader::LoadLine(fd)){
		if(*line == '\0' || *line == '#')
			continue;

		if(section == NONE){
			if(isLine4(line, 'o','b','j','s')) section = OBJS;
			else if(isLine4(line, 't','o','b','j')) section = TOBJ;
			else if(isLine4(line, 'w','e','a','p')) section = WEAP;
			else if(isLine4(line, 'h','i','e','r')) section = HIER;
			else if(isLine4(line, 'c','a','r','s')) section = CARS;
			else if(isLine4(line, 'p','e','d','s')) section = PEDS;
			else if(isLine4(line, 'p','a','t','h')) section = PATH;
			else if(isLine4(line, '2','d','f','x')) section = TWODFX;
		}else if(isLine3(line, 'e','n','d')){
			section = NONE;
		}else switch(section){
		case OBJS:
			id = LoadObject(line);
			if(id > maxID) maxID = id;
			if(id < minID) minID = id;
			break;
		case TOBJ:
			id = LoadTimeObject(line);
			if(id > maxID) maxID = id;
			if(id < minID) minID = id;
			break;
		case WEAP:
			LoadWeaponObject(line);
			break;
		case HIER:
			LoadClumpObject(line);
			break;
		case CARS:
			LoadVehicleObject(line);
			break;
		case PEDS:
			LoadPedObject(line);
			break;
		case PATH:
			if(pathIndex == -1){
				id = LoadPathHeader(line, pathType);
				pathIndex = 0;
			}else{
				if(pathType == 0)
					LoadPedPathNode(line, id, pathIndex);
				else if (pathType == 1)
					LoadCarPathNode(line, id, pathIndex, false);
				else if (pathType == 2)
					LoadCarPathNode(line, id, pathIndex, true);
				pathIndex++;
				if(pathIndex == 12)
					pathIndex = -1;
			}
			break;
		case TWODFX:
			Load2dEffect(line);
			break;
		}
	}
	CFileMgr::CloseFile(fd);

	for(id = minID; id <= maxID; id++){
		CSimpleModelInfo *mi = (CSimpleModelInfo*)CModelInfo::GetModelInfo(id);
		if(mi && mi->IsBuilding())
			mi->SetupBigBuilding(minID, maxID);
	}
}

#ifdef RW_DC
// PH3NOM - I just searched the extracted img folder for the following DFF names:
//find . -type f -name "*tree*.dff" ! -name "*shad*"
//find . -type f -name "*palm*.dff" ! -name "*shad*"
//find . -type f -name "*fence*.dff" ! -name "*shad*"
//find . -type f -name "*bush*.dff" ! -name "*shad*"
//find . -type f -name "*rail*.dff" ! -name "*shad*"
//find . -type f -name "*veg*.dff" ! -name "*shad*"
char *AlphaTestNames[] = {
    "BillBd04",
    "BillBd1",
    "BillBd2",
    "BillBd3",
    "LODk_grassarea",
    "LODmrgtrees4",
    "LODo_polgrnda10",
    "LODo_polgrnda12",
    "LODo_polgrnda13",
    "LODo_polgrnda14",
    "LODo_polgrnda16",
    "LODo_polgrnda17",
    "LODo_polgrnda7",
    "LODo_polgrnda8",
    "LODo_polgrnda9",
    "LODrtrees1",
    "LODrtrees2",
    "LODrtrees3",
    "LODrtrees4",
    "LODrtrees5",
    "LODse_pierfence",
    "LODst_woodfence30",
    "Man3_base_hedge",
    "Mansion2_C_fence",
    "Rose_Bush01",
    "Rose_Bush03",
    "Starhouse",
    "Streetlamp1",
    "Streetlamp2",
    "ap_blastdef_01",
    "ap_blastdef_03",
    "ap_boardshad1",
    "ap_carbush2_01",
    "ap_planters1_01",
    "ap_planters2_01",
    "ap_radar1_01",
    "ap_seafences",
    "ap_seaplanfence1",
    "ap_stairsout02",
    "ap_stairsout03",
    "ap_stairsout04",
    "ap_stairsout1",
    "ap_subfraightback01",
    "ap_subfraightback02",
    "ap_subfraightback04",
    "ap_treesfw1_01",
    "ap_treeshot1_02",
    "ap_wallfence1",
    "ap_wallfence2",
    "ap_wallfence3",
    "ap_wallfence4",
    "ap_wallfence5",
    "ap_wallfence6",
    "ap_wallfence7",
    "b_hse_pierfence",
    "bank_palms02",
    "bank_palms09",
    "bank_palms21",
    "bank_palms23",
    "bank_palms28",
    "bank_palms32",
    "bank_palms33",
    "basketballcourt04",
    "basketballcourt05",
    "bb_fence1",
    "beach_bush02",
    "beach_bush04",
    "beach_bush06s",
    "beach_bush08s",
    "biggrass2",
    "bldigste1mesh",
    "boatcranelg0",
    "build1",
    "buldingsite1",
    "carparkfence0",
    "chandelier",
    "ci_astagelights",
    "ci_astudganwal",
    "ci_boards1",
    "ci_boards2",
    "ci_busht_02",
    "ci_busht_04",
    "ci_busht_06",
    "ci_busht_08",
    "ci_busht_11",
    "ci_dstudiogantry",
    "ci_fence2",
    "ci_mans1rail",
    "ci_mans1rail1",
    "ci_mans2rail1",
    "ci_planter1",
    "ci_planter2",
    "ci_pornsetxtra",
    "ci_shipset",
    "ci_stairsout01",
    "ci_trees02",
    "ci_trees03",
    "ci_trees04",
    "ci_trees05",
    "ci_trees1",
    "cl_curtains",
    "cl_lightsuprts",
    "cl_railing",
    "cl_railingb",
    "cl_recessedlights1",
    "club_exterior01",
    "club_exterior02",
    "club_exterior06",
    "club_exterior07",
    "club_exterior08",
    "club_exterior09",
    "compound_fence",
    "concerth.col",
    "concerth02",
    "concerth03",
    "concerth04",
    "concerth05",
    "concerth06",
    "concerth07",
    "concerth08",
    "concerth09",
    "concerth10",
    "concerth11",
    "concerth12",
    "concerth27",
    "concerth29",
    "concerth48",
    "concerth49",
    "concerth50",
    "concerth51",
    "concerth52",
    "concerth54",
    "concerth56",
    "const_woodfence30",
    "cranebasea0",
    "crgoshp010",
    "crocneon",
    "deco_polgrnda10",
    "deco_polgrnda12",
    "deco_polgrnda13",
    "deco_polgrnda14",
    "deco_polgrnda16",
    "deco_polgrnda17",
    "deco_polgrnda7",
    "deco_polgrnda8",
    "deco_polgrnda9",
    "dk_cargoshp01",
    "dk_cargoshp03",
    "dk_cargoshp04",
    "dk_cargoshp05",
    "dk_cargoshp10",
    "dk_cargoshp100",
    "dk_cargoshp12",
    "dk_cargoshp24",
    "dk_cargoshp25",
    "dk_cargoshp28",
    "dk_cargoshp31",
    "dk_cargoshp32",
    "dk_cargoshp35",
    "dk_cargoshp40",
    "dk_cargoshp41",
    "dk_cargoshp47",
    "dk_cargoshp50",
    "dk_cargoshp51",
    "dk_cargoshp53",
    "dk_cargoshp54",
    "dk_cargoshp64",
    "dk_cargoshp65",
    "dk_cargoshp66",
    "dk_cargoshp68",
    "dk_cargoshp70",
    "dk_cargoshp71",
    "dk_cargoshp72",
    "dk_cargoshp73",
    "dk_cargoshp76",
    "dk_cargoshp95",
    "dk_rail01",
    "dk_rail02",
    "dk_rail03",
    "dk_rail03",	
    "dk_rail04",
    "dk_rail05",
    "dk_rail06",
    "dk_rail07",
    "doc_crane_cab0",
    "doc_crane_cab01",
    "doc_crane_cab02",
    "doc_crane_cab03",
    "doc_crane_cab04",
    "doc_craneeggs04",
    "dock_grassarea",
    "dock_props01",
    "dock_props02",
    "dockcranescale0",
    "dockcranescale01",
    "dockfence",
    "dockgate01",
    "dockgate02",
    "dockgrass",
    "docks10",
    "docks10rail",
    "docksware01",
    "doontoon18",
    "doontoon25_alfas",
    "doontoon32_alfa",
    "doontoon66_bushes01",
    "doontoon76",
    "doublestreetlght1",
    "drive_flowers06",
    "dt_compound_alfas",
    "dt_scabby_fence01",
    "dt_scabby_fence03",
    "dt_scabby_fence04",
    "dt_scabby_fence07",
    "dt_sheraton_fence",
    "dt_stadium__alfa1",
    "dtn_cablesa",
    "dtn_cablesb",
    "dtn_cablesc",
    "dtn_cablesd",
    "dtn_cablese",
    "dtn_cablesf",
    "dtn_hospital_fence",
    "dtn_veg3",
    "dtn_veg4",
    "dts_telwire1",
    "dts_telwire2",
    "dts_telwire4",
    "dts_telwire9",
    "dzplant",
    "fence01",
    "fencehaiti",
    "fencehaitism",
    "fencesmallb",
    "gf_drdivide1_01",
    "gf_drivingrange1",
    "gf_golfwall",
    "gf_picket1_01",
    "gf_planters2",
    "gf_planters4",
    "gf_stadlight1_02",
    "gf_tennisfence",
    "gf_tree1_01",
    "gf_tree2_02",
    "gf_treesfw1_01",
    "gf_treesfw2_01",
    "gf_treesfw3_01",
    "gf_treesfw4_01",
    "gf_treesfw5_01",
    "grassbitsjm",
    "grasspatch",
    "haitinwire11",
    "haitinwire22",
    "haitinwire23",
    "havanahouse12b",
    "hoteltrees01",
    "hoteltrees02",
    "hoteltrees03",
    "hoteltrees05",
    "hoteltrees06",
    "ht_veg01_dy",
    "ht_veg01_nt",
    "ht_veg02_dy",
    "ht_veg02_nt",
    "ht_veg04_dy",
    "ht_veg04_nt",
    "kb_planter",
    "kb_planter+bush",
    "kb_planter+bush2",
    "kb_planterbox",
    "kb_planterbush2",
    "kickfence",
    "labiggrass",
    "labiggrass01",
    "labiggrass02",
    "lh_haiblockc3",
    "lha_carfence",
    "lhavnew_bush",
    "lhbasket",
    "lhbillboard05xx",
    "lhroofst01",
    "lhroofst02",
    "lhroofst03",
    "lhroofst04",
    "lhroofst07",
    "lhroofst08",
    "lhroofst09",
    "lhroofst10",
    "lhroofst11",
    "lhroofst12",
    "lhroofst14",
    "lhroofst16",
    "lhroofst17",
    "lhroofst18",
    "lhroofst19",
    "lhroofstuffg",
    "lhsteps3",
    "lithavabit01",
    "lithavabit03",
    "lithavabit04",
    "lithavabit05",
    "lithavabit06",
    "lithavabit07",
    "lithavabit08",
    "lithavabit09",
    "lithavabit10",
    "lithavabit11",
    "lithavabit12",
    "littlehacoast02",
    "littlehacoast05",
    "lw_palm1",
    "mallbushdense",
    "mallbushs",
    "malltreereflect",
    "malltreereflect2",
    "malltrees01",
    "man_build01",
    "man_build01_al",
    "man_build02",
    "man_build02_al",
    "man_build03",
    "man_build03_al",
    "man_build04",
    "man_build04_fnce",
    "man_build05",
    "man_build05_al",
    "man_build_int",
    "man_chandeliers",
    "man_f_chandelier",
    "man_lights_pool",
    "man_pool",
    "man_safenewmny",
    "man_scar_curtains",
    "man_scar_props206",
    "man_sdr_folge",
    "man_sdr_rug",
    "man_twr_stairs",
    "man_twr_stairsB",
    "man_xchandeliers",
    "mansbushes",
    "mansbushes2",
    "mansion1b",
    "mansionfence",
    "marinafence01",
    "marinafence02",
    "marinafence03",
    "marinafence04",
    "marinafence05",
    "marinaplanter1",
    "marinaplanter2",
    "marinaveg1",
    "marinaveg2",
    "marinaveg3",
    "mc_fence1",
    "mc_fence2",
    "mc_fence3",
    "mc_largebuild",
    "mc_tower_01",
    "mc_tower_02",
    "mc_tower_03",
    "mc_treesfw1_01",
    "mc_treesfw2_01",
    "mc_treesfw3_01",
    "mc_wall",
    "mc_wall2",
    "mc_wall3",
    "ml_planterbed",
    "ml_vegbits01",
    "ml_vegbits02",
    "ml_vegbits03",
    "ml_vegbits04",
    "ml_vegbits05",
    "mlmallbush",
    "mn_treesis_int",
    "mn_treesisl05",
    "nbchflw01",
    "nbchflw02",
    "nbdcprmndfnc1",
    "nbdcprmndfnc2",
    "nbdecoshplants",
    "nbeachbed01",
    "nbeachbed02",
    "nbeachbit03rails",
    "nbeachvegy1",
    "nbeachvegy2",
    "nbecland12",
    "nbecland12bball",
    "nbecland12gates",
    "nbt_balcony02",
    "nbt_balcony03",
    "nbt_balcony05",
    "nbt_barfence01",
    "nbt_hotel06balcony",
    "nbt_hotel07fence",
    "nbt_hoteltrees01",
    "nbt_hoteltrees02",
    "nbt_mansion01",
    "nbt_mansion02",
    "nbt_mansion03",
    "nbt_mansion04",
    "nbt_mansion0402",
    "nbt_mansiontrees01",
    "nbt_mansiontrees02",
    "nbt_mansiontrees03",
    "nbt_mansiontrees04",
    "nbt_mansiontrees05",
    "nbt_rooftopstart",
    "nbthotel08fence02",
    "nbw_bush01",
    "nbw_bush02",
    "nbwbusshy2",
    "new_bushsm",
    "new_bushtest",
    "new_bushtest42",
    "newbuild01",
    "newbuild02",
    "nhaitiplnt14",
    "nhaitiplnt15",
    "nrth1veg21",
    "nrth1veg37",
    "nrth1veg42",
    "nrth3veg05",
    "nrth3veg08",
    "nrth3veg16",
    "nrth3veg25",
    "nrth3veg35",
    "nrth3veg50",
    "nrth3veg59",
    "nrth4veg05",
    "nrth4veg08",
    "nrth4veg09",
    "nrth4veg21",
    "nrth4veg212",
    "nrth7veg",
    "nrth7veg09",
    "nt_cablebox1_01",
    "nt_cablebox2_01",
    "nt_cablebox3_01",
    "nt_cablebox4_01",
    "nt_cablebox5_01",
    "nt_cablebox6_01",
    "od_alleys1_01_dy",
    "od_alleys1_01_nt",
    "od_alleys1b_01_dy",
    "od_alleys1b_01_nt",
    "od_alleys2_01_dy",
    "od_alleys2_01_nt",
    "od_alleys3_01_dy",
    "od_alleys3_01_nt",
    "od_alleys3b_01_dy",
    "od_alleys3b_01_nt",
    "od_bighotsign01",
    "od_clevelfence",
    "od_clubfence_dy",
    "od_clubfence_nt",
    "oddoorway",
    "oddoorway2",
    "oddoorwayS02",
    "oddoorwayS1",
    "odnvegbush1",
    "odnvegbush2",
    "odnvegbush2b",
    "odnwiret",
    "odrampbit",
    "odrroofst02",
    "odrroofst06",
    "odrroofst07",
    "odrv_bushes",
    "odrv_bushes01",
    "pierentrance02",
    "plants01",
    "plants04",
    "plants05",
    "plants05b",
    "pw_backfence1",
    "pw_bayfence2_01",
    "rustship_structure0",
    "scrapgate",
    "sexgarden2",
    "shipstairs",
    "shpfrnts03rail01",
    "shpfrnts03rail02",
    "sjmgrass",
    "sjmgrass2",
    "spad_veg1",
    "starbbnet02",
    "starbitbush",
    "starbits05",
    "starbits06",
    "starbits13",
    "starbits13b",
    "starbits13c",
    "starbits17",
    "starbits20",
    "starbits22",
    "starbits24",
    "starbits45",
    "starbits46",
    "starbits49",
    "starbits50",
    "starbits51",
    "starbits52",
    "starbits54",
    "starbits55",
    "starbits56",
    "starbits57",
    "starbits58",
    "starbits59",
    "starblocks04",
    "stardepot_FENCE",
    "starfbed02",
    "starfbed03",
    "starfbed04",
    "starfbed05",
    "starfbed06",
    "starsbush2",
    "starwaterfrnt08",
    "stationfence",
    "stripinter",
    "stripinterupside",
    "striplights",
    "svegrgedoor",
    "tall_fence",
    "telewire02",
    "telewire03",
    "telewire04",
    "telgrphpole02",
    "tesad06",
    "tesad06sas",
    "tesad06ss",
    "veg_fern_balcny_kb1",
    "veg_fern_balcny_kb2",
    "veg_gaz",
    "veg_ivy_balcny_kb3",
    "veg_palm01",
    "veg_palm02",
    "veg_palm03",
    "veg_palm04",
    "veg_palmbig14",
    "veg_palmkb1",
    "veg_palmkb10",
    "veg_palmkb13",
    "veg_palmkb14",
    "veg_palmkb1pERC",
    "veg_palmkb2",
    "veg_palmkb3",
    "veg_palmkb4",
    "veg_palmkb5",
    "veg_palmkb7",
    "veg_palmkb8",
    "veg_palmkb9",
    "veg_palmkbb11",
    "veg_palwee01",
    "veg_palwee02",
    "veg_palwee03",
    "veg_tree3",
    "veg_treea1",
    "veg_treea3",
    "veg_treeb1",
    "veged",
    "veged01",
    "veged02",
    "vegeha1",
    "vegetationb",
    "vegetationb03",
    "washbuild003",
    "washbuild005",
    "washbuild013",
    "washbuild014",
    "washbuild015",
    "washbuild017",
    "washbuild018",
    "washbuild020",
    "washbuild021",
    "washbuild025",
    "washbuild025neon",
    "washbuild030",
    "washbuild033",
    "washbuild039",
    "washbuild040",
    "washbuild041",
    "washbuild043",
    "washbuild050",
    "washbuild05067",
    "washbuild058",
    "washbuild062",
    "washbuild063",
    "washbuild066",
    "washbuild068",
    "washbuild069",
    "washbuild071",
    "washbuild072",
    "washbuild073",
    "washbuild074",
    "washbuild075",
    "washbuild081",
    "washbuild0812",
    "washbuild111",
    "washbuild112",
    "washbuild114",
    "washbuild116",
    "washbuild119",
    "washbuild120",
    "washbuild121",
    "washbuild184",
    "washbuild187",
    "washbuild191",
    "washbuild192",
    "washbuild194",
    "washbuild195",
    "washbuild198",
    "washbuild199",
    "washbuild200",
    "washbuild201",
    "washbuild202",
    "washbuild203",
    "washbuild206",
    "washbuild210",
    "washbuild213",
    "washbuild214",
    "washbuild215",
    "washbuild216",
    "washbuild217",
    "washbuild231",
    "washelipad",
    "washfence1",
    "washfence2",
    "washpshoutdet",
    "washpshoutveg",
    "washskyplant1",
    "washskyplant2",
    "washundermall1",
    "washvegy2",
    "washvegy237",
    "washvegy238",
    "washvegy239",
    "washvegy240",
    "washvegy241",
    "washvegy2413",
    "washvegy242",
    "washvegy3",
    "washvegy4",
    "woshmallglas1",
    "woshmallglas2",
    "wsgbuildwl97",
    "wshbuildws42",
    "wshotelsign35",
    "wshotelveg1",
    "wshscarland226",
    "wshtelgrphcabl01",
    "wshtelgrphcabl02",
    "wshtelgrphcabl03",
    "wshtelgrphcabl04",
    "wshtelgrphcabl05",
    "wshtelgrphcabl06",
    "wshtelgrphcabl07",
    "wshtelgrphcabl08",
    "wshtelgrphcabl09",
    "wshtelgrphcabl10",
    "wshtelgrphcabl11",
    "xpolytrees1_dt",
    "xpolytrees2_dt",
    "xpolytrees3_dt",
    "xpolytrees4_dt",

	"",
};

static bool
MatchModelName(char *name, char **list)
{
	int i;
	char *s;
	for(i = 0; *list[i] != '\0'; i++)
		if(strncmp(name, "LOD", 3) == 0){
			if(!CGeneral::faststricmp(name+3, list[i]+3))
				return true;
		}else{
			if(!CGeneral::faststricmp(name, list[i]))
				return true;
		}
	return false;
}
#endif

void
SetModelInfoFlags(CSimpleModelInfo *mi, uint32 flags)
{
	mi->m_wetRoadReflection =	!!(flags & 1);
	mi->m_noFade =	!!(flags & 2);
	mi->m_drawLast =	!!(flags & (4|8));
	mi->m_additive =	!!(flags & 8);
	mi->m_isSubway =	!!(flags & 0x10);
	mi->m_ignoreLight =	!!(flags & 0x20);
	mi->m_noZwrite =	!!(flags & 0x40);
	mi->m_noShadows =	!!(flags & 0x80);
	mi->m_ignoreDrawDist =	!!(flags & 0x100);
	mi->m_isCodeGlass =	!!(flags & 0x200);
	mi->m_isArtistGlass =	!!(flags & 0x400);

#ifdef RW_DC
	if(MatchModelName(mi->GetModelName(), AlphaTestNames)) mi->m_isAlphaTest = true;
#endif	
}

int
CFileLoader::LoadObject(const char *line)
{
	int id, numObjs;
	char model[24], txd[24];
	float dist[3];
	uint32 flags;
	int damaged;
	CSimpleModelInfo *mi;

	if(sscanf(line, "%d %s %s %d", &id, model, txd, &numObjs) != 4)
		return 0;	// game returns return value

	switch(numObjs){
	case 1:
		sscanf(line, "%d %s %s %d %f %d",
			&id, model, txd, &numObjs, &dist[0], &flags);
		damaged = 0;
		break;
	case 2:
		sscanf(line, "%d %s %s %d %f %f %d",
			&id, model, txd, &numObjs, &dist[0], &dist[1], &flags);
		damaged = dist[0] < dist[1] ?	// Are distances increasing?
			0 :	// Yes, no damage model
			1;	// No, 1 is damaged
		break;
	case 3:
		sscanf(line, "%d %s %s %d %f %f %f %d",
			&id, model, txd, &numObjs, &dist[0], &dist[1], &dist[2], &flags);
		damaged = dist[0] < dist[1] ?	// Are distances increasing?
				(dist[1] < dist[2] ? 0 : 2) :	// Yes, only 2 can still be a damage model
			1;	// No, 1 and 2 are damaged
		break;
	}

	mi = CModelInfo::AddSimpleModel(id);
	mi->SetModelName(model);
	mi->SetNumAtomics(numObjs);
	mi->SetLodDistances(dist);
	SetModelInfoFlags(mi, flags);
	mi->m_firstDamaged = damaged;
	mi->SetTexDictionary(txd);
	MatchModelString(model, id);

	return id;
}

int
CFileLoader::LoadTimeObject(const char *line)
{
	int id, numObjs;
	char model[24], txd[24];
	float dist[3];
	uint32 flags;
	int timeOn, timeOff;
	int damaged;
	CTimeModelInfo *mi, *other;

	if(sscanf(line, "%d %s %s %d", &id, model, txd, &numObjs) != 4)
		return 0;	// game returns return value

	switch(numObjs){
	case 1:
		sscanf(line, "%d %s %s %d %f %d %d %d",
			&id, model, txd, &numObjs, &dist[0], &flags, &timeOn, &timeOff);
		damaged = 0;
		break;
	case 2:
		sscanf(line, "%d %s %s %d %f %f %d %d %d",
			&id, model, txd, &numObjs, &dist[0], &dist[1], &flags, &timeOn, &timeOff);
		damaged = dist[0] < dist[1] ?	// Are distances increasing?
			0 :	// Yes, no damage model
			1;	// No, 1 is damaged
		break;
	case 3:
		sscanf(line, "%d %s %s %d %f %f %f %d %d %d",
			&id, model, txd, &numObjs, &dist[0], &dist[1], &dist[2], &flags, &timeOn, &timeOff);
		damaged = dist[0] < dist[1] ?	// Are distances increasing?
				(dist[1] < dist[2] ? 0 : 2) :	// Yes, only 2 can still be a damage model
			1;	// No, 1 and 2 are damaged
		break;
	}

	mi = CModelInfo::AddTimeModel(id);
	mi->SetModelName(model);
	mi->SetNumAtomics(numObjs);
	mi->SetLodDistances(dist);
	SetModelInfoFlags(mi, flags);
	mi->m_firstDamaged = damaged;
	mi->SetTimes(timeOn, timeOff);
	mi->SetTexDictionary(txd);
	other = mi->FindOtherTimeModel();
	if(other)
		other->SetOtherTimeModel(id);
	MatchModelString(model, id);

	return id;
}

int
CFileLoader::LoadWeaponObject(const char *line)
{
	int id, numObjs;
	char model[24], txd[24], animFile[16];
	float dist;
	CWeaponModelInfo *mi;

	sscanf(line, "%d %s %s %s %d %f", &id, model, txd, animFile, &numObjs, &dist);

	mi = CModelInfo::AddWeaponModel(id);
	mi->SetModelName(model);
	mi->SetNumAtomics(1);
	mi->m_lodDistances[0] = dist;
	mi->SetTexDictionary(txd);
	mi->SetAnimFile(animFile);
	mi->SetColModel(&CTempColModels::ms_colModelWeapon);
	MatchModelString(model, id);
	return id;
}

void
CFileLoader::LoadClumpObject(const char *line)
{
	int id;
	char model[24], txd[24];
	CClumpModelInfo *mi;

	if(sscanf(line, "%d %s %s", &id, model, txd) == 3){
		mi = CModelInfo::AddClumpModel(id);
		mi->SetModelName(model);
		mi->SetTexDictionary(txd);
		mi->SetColModel(&CTempColModels::ms_colModelBBox);
	}
}

void
CFileLoader::LoadVehicleObject(const char *line)
{
	int id;
	char model[24], txd[24];
	char type[8], handlingId[16], gamename[32], animFile[16], vehclass[12];
	uint32 frequency, comprules;
	int32 level, misc;
	float wheelScale;
	CVehicleModelInfo *mi;
	char *p;

	sscanf(line, "%d %s %s %s %s %s %s %s %d %d %x %d %f",
		&id, model, txd,
		type, handlingId, gamename, animFile, vehclass,
		&frequency, &level, &comprules, &misc, &wheelScale);

	mi = CModelInfo::AddVehicleModel(id);
	mi->SetModelName(model);
	mi->SetTexDictionary(txd);
	mi->SetAnimFile(animFile);
	for(p = gamename; *p; p++)
		if(*p == '_') *p = ' ';
	strcpy(mi->m_gameName, gamename);
	mi->m_level = level;
	mi->m_compRules = comprules;

	if(strcmp(type, "car") == 0){
		mi->m_wheelId = misc;
		mi->m_wheelScale = wheelScale;
		mi->m_vehicleType = VEHICLE_TYPE_CAR;
	}else if(strcmp(type, "boat") == 0){
		mi->m_vehicleType = VEHICLE_TYPE_BOAT;
	}else if(strcmp(type, "train") == 0){
		mi->m_vehicleType = VEHICLE_TYPE_TRAIN;
	}else if(strcmp(type, "heli") == 0){
		mi->m_vehicleType = VEHICLE_TYPE_HELI;
	}else if(strcmp(type, "plane") == 0){
		mi->m_planeLodId = misc;
		mi->m_wheelScale = 1.0f;
		mi->m_vehicleType = VEHICLE_TYPE_PLANE;
	}else if(strcmp(type, "bike") == 0){
		mi->m_bikeSteerAngle = misc;
		mi->m_wheelScale = wheelScale;
		mi->m_vehicleType = VEHICLE_TYPE_BIKE;
	}else
		assert(0);

	mi->m_handlingId = mod_HandlingManager.GetHandlingId(handlingId);

	if(strcmp(vehclass, "normal") == 0)
		mi->m_vehicleClass = CCarCtrl::NORMAL;
	else if(strcmp(vehclass, "poorfamily") == 0)
		mi->m_vehicleClass = CCarCtrl::POOR;
	else if(strcmp(vehclass, "richfamily") == 0)
		mi->m_vehicleClass = CCarCtrl::RICH;
	else if(strcmp(vehclass, "executive") == 0)
		mi->m_vehicleClass = CCarCtrl::EXEC;
	else if(strcmp(vehclass, "worker") == 0)
		mi->m_vehicleClass = CCarCtrl::WORKER;
	else if(strcmp(vehclass, "big") == 0)
		mi->m_vehicleClass = CCarCtrl::BIG;
	else if(strcmp(vehclass, "taxi") == 0)
		mi->m_vehicleClass = CCarCtrl::TAXI;
	else if(strcmp(vehclass, "moped") == 0)
		mi->m_vehicleClass = CCarCtrl::MOPED;
	else if(strcmp(vehclass, "motorbike") == 0)
		mi->m_vehicleClass = CCarCtrl::MOTORBIKE;
	else if(strcmp(vehclass, "leisureboat") == 0)
		mi->m_vehicleClass = CCarCtrl::LEISUREBOAT;
	else if(strcmp(vehclass, "workerboat") == 0)
		mi->m_vehicleClass = CCarCtrl::WORKERBOAT;
	else if(strcmp(vehclass, "ignore") == 0) {
		mi->m_vehicleClass = -1;
		return;
	}
	CCarCtrl::AddToCarArray(id, mi->m_vehicleClass);
	mi->m_frequency = frequency;
}

void
CFileLoader::LoadPedObject(const char *line)
{
	int id;
	char model[24], txd[24];
	char pedType[24], pedStats[24], animGroup[24], animFile[16];
	int carsCanDrive;
	CPedModelInfo *mi;
	int animGroupId;
	int radio1, radio2;

	sscanf(line, "%d %s %s %s %s %s %x %s %d %d",
	          &id, model, txd,
	          pedType, pedStats, animGroup, &carsCanDrive,
		  animFile, &radio1, &radio2);

	mi = CModelInfo::AddPedModel(id);
	mi->SetModelName(model);
	mi->SetTexDictionary(txd);
	mi->SetAnimFile(animFile);
	mi->SetColModel(&CTempColModels::ms_colModelPed1);
	mi->m_pedType = CPedType::FindPedType(pedType);
	mi->m_pedStatType = CPedStats::GetPedStatType(pedStats);
	for(animGroupId = 0; animGroupId < NUM_ANIM_ASSOC_GROUPS; animGroupId++)
		if(strcmp(animGroup, CAnimManager::GetAnimGroupName((AssocGroupId)animGroupId)) == 0)
			break;
	assert(animGroupId < NUM_ANIM_ASSOC_GROUPS);
	mi->m_animGroup = animGroupId;
	mi->m_carsCanDrive = carsCanDrive;
	mi->radio1 = radio1;
	mi->radio2 = radio2;
}

int
CFileLoader::LoadPathHeader(const char *line, int &type)
{
	int id;
	char modelname[32];

	sscanf(line, "%d %d %s", &type, &id, modelname);
	return id;
}

void
CFileLoader::LoadPedPathNode(const char *line, int id, int node)
{
	int type, next, cross, numLeft, numRight, speed, flags;
	float x, y, z, width, spawnRate;

	if(sscanf(line, "%d %d %d %f %f %f %f %d %d %d %d %f",
			&type, &next, &cross, &x, &y, &z, &width, &numLeft, &numRight,
			&speed, &flags, &spawnRate) != 12)
		spawnRate = 1.0f;

	if(id == -1)
		ThePaths.StoreDetachedNodeInfoPed(node, type, next, x, y, z,
			width, !!cross, !!(flags&1), !!(flags&4), spawnRate*15.0f);
	else
		ThePaths.StoreNodeInfoPed(id, node, type, next, x, y, z,
			width, !!cross, spawnRate*15.0f);
}

void
CFileLoader::LoadCarPathNode(const char *line, int id, int node, bool waterPath)
{
	int type, next, cross, numLeft, numRight, speed, flags;
	float x, y, z, width, spawnRate;

	if(sscanf(line, "%d %d %d %f %f %f %f %d %d %d %d %f",
			&type, &next, &cross, &x, &y, &z, &width, &numLeft, &numRight,
			&speed, &flags, &spawnRate) != 12)
		spawnRate = 1.0f;

	if(id == -1)
		ThePaths.StoreDetachedNodeInfoCar(node, type, next, x, y, z, width, numLeft, numRight,
			!!(flags&1), !!(flags&4), speed, !!(flags&2), waterPath, spawnRate * 15, false);
	else
		ThePaths.StoreNodeInfoCar(id, node, type, next, x, y, z, 0, numLeft, numRight,
			!!(flags&1), !!(flags&4), speed, !!(flags&2), waterPath, spawnRate * 15);
}


void
CFileLoader::Load2dEffect(const char *line)
{
	int id, r, g, b, a, type, ptype;
	float x, y, z;
	char corona[32], shadow[32];
	int shadowIntens, lightType, roadReflection, flare, flags, probability;
	CBaseModelInfo *mi;
	C2dEffect *effect;
	char *p;

	sscanf(line, "%d %f %f %f %d %d %d %d %d", &id, &x, &y, &z, &r, &g, &b, &a, &type);

	CTxdStore::PushCurrentTxd();
	CTxdStore::SetCurrentTxd(CTxdStore::FindTxdSlot("particle"));

	mi = CModelInfo::GetModelInfo(id);
	effect = CModelInfo::Get2dEffectStore().Alloc();
	mi->Add2dEffect(effect);
	effect->pos = CVector(x, y, z);
	effect->col = CRGBA(r, g, b, a);
	effect->type = type;

	switch(effect->type){
	case EFFECT_LIGHT:
		while(*line++ != '"');
		p = corona;
		while(*line != '"') *p++ = *line++;
		*p = '\0';
		line++;

		while(*line++ != '"');
		p = shadow;
		while(*line != '"') *p++ = *line++;
		*p = '\0';
		line++;

		sscanf(line, "%f %f %f %f %d %d %d %d %d",
			&effect->light.dist,
			&effect->light.range,
			&effect->light.size,
			&effect->light.shadowSize,
			&shadowIntens, &lightType, &roadReflection, &flare, &flags);
		effect->light.corona = RwTextureRead(corona, nil);
		effect->light.shadow = RwTextureRead(shadow, nil);
		effect->light.shadowIntensity = shadowIntens;
		effect->light.lightType = lightType;
		effect->light.roadReflection = roadReflection;
		effect->light.flareType = flare;

		if(flags & LIGHTFLAG_FOG_ALWAYS)
			flags &= ~LIGHTFLAG_FOG_NORMAL;
		effect->light.flags = flags;
		break;

	case EFFECT_PARTICLE:
		sscanf(line, "%d %f %f %f %d %d %d %d %d %d %f %f %f %f",
			&id, &x, &y, &z, &r, &g, &b, &a, &type,
			&effect->particle.particleType,
			&effect->particle.dir.x,
			&effect->particle.dir.y,
			&effect->particle.dir.z,
			&effect->particle.scale);
		break;

	case EFFECT_ATTRACTOR:
		sscanf(line, "%d %f %f %f %d %d %d %d %d %d %f %f %f %d",
			&id, &x, &y, &z, &r, &g, &b, &a, &type,
			&flags,
			&effect->attractor.dir.x,
			&effect->attractor.dir.y,
			&effect->attractor.dir.z,
			&probability);
		effect->attractor.type = flags;
#ifdef FIX_BUGS
		effect->attractor.probability = Clamp(probability, 0, 255);
#else
		effect->attractor.probability = probability;
#endif
		break;
	case EFFECT_PED_ATTRACTOR:
		sscanf(line, "%d %f %f %f %d %d %d %d %d %d %f %f %f %f %f %f",
			&id, &x, &y, &z, &r, &g, &b, &a, &type,
			&ptype,
			&effect->pedattr.queueDir.x,
			&effect->pedattr.queueDir.y,
			&effect->pedattr.queueDir.z,
			&effect->pedattr.useDir.x,
			&effect->pedattr.useDir.y,
			&effect->pedattr.useDir.z);
		effect->pedattr.type = ptype;
		break;
	}

	CTxdStore::PopCurrentTxd();
}

void
CFileLoader::LoadScene(const char *filename)
{
	enum {
		NONE,
		INST,
		ZONE,
		CULL,
		OCCL,
		PICK,
		PATH,
	};
	char *line;
	int fd;
	int section;
	int pathType, pathIndex;

	section = NONE;
	pathIndex = -1;
	debug("Creating objects from %s...\n", filename);

	fd = CFileMgr::OpenFile(filename, "rb");
	assert(fd > 0);
	for(line = CFileLoader::LoadLine(fd); line; line = CFileLoader::LoadLine(fd)){
		if(*line == '\0' || *line == '#')
			continue;

		if(section == NONE){
			if(isLine4(line, 'i','n','s','t')) section = INST;
			else if(isLine4(line, 'z','o','n','e')) section = ZONE;
			else if(isLine4(line, 'c','u','l','l')) section = CULL;
			else if(isLine4(line, 'p','i','c','k')) section = PICK;
			else if(isLine4(line, 'p','a','t','h')) section = PATH;
			else if(isLine4(line, 'o','c','c','l')) section = OCCL;
		}else if(isLine3(line, 'e','n','d')){
			section = NONE;
		}else switch(section){
		case INST:
			LoadObjectInstance(line);
			break;
		case ZONE:
			LoadZone(line);
			break;
		case CULL:
			LoadCullZone(line);
			break;
		case OCCL:
			LoadOcclusionVolume(line);
			break;
		case PICK:
			// unused
			LoadPickup(line);
			break;
		case PATH:
			if(pathIndex == -1){
				LoadPathHeader(line, pathType);
				pathIndex = 0;
			}else{
				if(pathType == 0)
					LoadPedPathNode(line, -1, pathIndex);
				else if (pathType == 1)
					LoadCarPathNode(line, -1, pathIndex, false);
				else if (pathType == 2)
					LoadCarPathNode(line, -1, pathIndex, true);
				pathIndex++;
				if(pathIndex == 12)
					pathIndex = -1;
			}
			break;
		}
	}
	CFileMgr::CloseFile(fd);

	debug("Finished loading IPL\n");
}

void
CFileLoader::LoadObjectInstance(const char *line)
{
	int id;
	char name[24];
	RwV3d trans, scale, axis;
	float angle;
	CSimpleModelInfo *mi;
	RwMatrix *xform;
	CEntity *entity;
	float area;

	if(sscanf(line, "%d %s %f %f %f %f %f %f %f %f %f %f %f",
	          &id, name, &area,
	          &trans.x, &trans.y, &trans.z,
	          &scale.x, &scale.y, &scale.z,
	          &axis.x, &axis.y, &axis.z, &angle) != 13){
		if(sscanf(line, "%d %s %f %f %f %f %f %f %f %f %f %f",
		          &id, name,
		          &trans.x, &trans.y, &trans.z,
		          &scale.x, &scale.y, &scale.z,
		          &axis.x, &axis.y, &axis.z, &angle) != 12)
			return;
		area = 0;
	}

	mi = (CSimpleModelInfo*)CModelInfo::GetModelInfo(id);
	if(mi == nil)
		return;
	assert(mi->IsSimple());

	if(!CStreaming::IsObjectInCdImage(id))
		debug("Not in cdimage %s\n", mi->GetModelName());

	angle = -RADTODEG(2.0f * Acos(angle));
	xform = RwMatrixCreate();
	RwMatrixRotate(xform, &axis, angle, rwCOMBINEREPLACE);
	RwMatrixTranslate(xform, &trans, rwCOMBINEPOSTCONCAT);

	if(mi->GetObjectID() == -1){
		if(ThePaths.IsPathObject(id)){
			entity = new CTreadable;
			ThePaths.RegisterMapObject((CTreadable*)entity);
		}else
			entity = new CBuilding;
		entity->SetModelIndexNoCreate(id);
		entity->GetMatrix() = CMatrix(xform);
		entity->m_level = CTheZones::GetLevelFromPosition(&entity->GetPosition());
		entity->m_area = area;
		if(mi->IsBuilding()){
			if(mi->m_isBigBuilding)
				entity->SetupBigBuilding();
			if(mi->m_isSubway)
				entity->bIsSubway = true;
		}
		if(mi->GetLargestLodDistance() < 2.0f)
			entity->bIsVisible = false;
		CWorld::Add(entity);

		CColModel *col = entity->GetColModel();
		if(col->numSpheres || col->numBoxes || col->numTriangles){
			if(col->level != 0)
				CColStore::GetBoundingBox(col->level).ContainRect(entity->GetBoundRect());
		}else
			entity->bUsesCollision = false;

		if(entity->GetPosition().z + col->boundingBox.min.z < 6.0f)
			entity->bUnderwater = true;
	}else{
		entity = new CDummyObject;
		entity->SetModelIndexNoCreate(id);
		entity->GetMatrix() = CMatrix(xform);
		CWorld::Add(entity);
		if(IsGlass(entity->GetModelIndex()) && !mi->m_isArtistGlass)
			entity->bIsVisible = false;
		entity->m_level = CTheZones::GetLevelFromPosition(&entity->GetPosition());
		entity->m_area = area;
	}

	RwMatrixDestroy(xform);
}

void
CFileLoader::LoadZone(const char *line)
{
	char name[24];
	int type, level;
	float minx, miny, minz;
	float maxx, maxy, maxz;

	if(sscanf(line, "%s %d %f %f %f %f %f %f %d", name, &type, &minx, &miny, &minz, &maxx, &maxy, &maxz, &level) == 9)
		CTheZones::CreateZone(name, (eZoneType)type, minx, miny, minz, maxx, maxy, maxz, (eLevelName)level);
}

void
CFileLoader::LoadCullZone(const char *line)
{
	CVector pos;
	float minx, miny, minz;
	float maxx, maxy, maxz;
	int flags;
	int wantedLevelDrop = 0;

	sscanf(line, "%f %f %f %f %f %f %f %f %f %d %d",
		&pos.x, &pos.y, &pos.z,
		&minx, &miny, &minz,
		&maxx, &maxy, &maxz,
		&flags, &wantedLevelDrop);
	CCullZones::AddCullZone(pos, minx, maxx, miny, maxy, minz, maxz, flags, wantedLevelDrop);
}

// unused
void
CFileLoader::LoadPickup(const char *line)
{
	int id;
	float x, y, z;

	sscanf(line, "%d %f %f %f", &id, &x, &y, &z);
}

void
CFileLoader::LoadOcclusionVolume(const char *line)
{
	float x, y, z;
	float width, length, height;
	float angle;

	sscanf(line, "%f %f %f %f %f %f %f",
		&x, &y, &z,
		&width, &length, &height,
		&angle);
	COcclusion::AddOne(x, y, z + height/2.0f, width, length, height, angle);
}


// unused
void
CFileLoader::ReloadPaths(const char *filename)
{
	enum {
		NONE,
		PATH,
	};
	char *line;
	int section = NONE;
	int id, pathType, pathIndex = -1;
	debug("Reloading paths from %s...\n", filename);

	int fd = CFileMgr::OpenFile(filename, "r");
	assert(fd > 0);
	for (line = CFileLoader::LoadLine(fd); line; line = CFileLoader::LoadLine(fd)) {
		if (*line == '\0' || *line == '#')
			continue;

		if (section == NONE) {
			if (isLine4(line, 'p','a','t','h')) {
				section = PATH;
				ThePaths.AllocatePathFindInfoMem(4500);
			}
		} else if (isLine3(line, 'e','n','d')) {
			section = NONE;
		} else {
			switch (section) {
				case PATH:
					if (pathIndex == -1) {
						id = LoadPathHeader(line, pathType);
						pathIndex = 0;
					} else {
						if(pathType == 0)
							LoadPedPathNode(line, id, pathIndex);
						else if (pathType == 1)
							LoadCarPathNode(line, id, pathIndex, false);
						else if (pathType == 2)
							LoadCarPathNode(line, id, pathIndex, true);
						pathIndex++;
						if (pathIndex == 12)
							pathIndex = -1;
					}
					break;
				default:
					break;
			}
		}
	}
	CFileMgr::CloseFile(fd);
}

void
CFileLoader::ReloadObjectTypes(const char *filename)
{
	enum {
		NONE,
		OBJS,
		TOBJ,
		TWODFX
	};
	char *line;
	int section = NONE;
	CModelInfo::ReInit2dEffects();
	debug("Reloading object types from %s...\n", filename);

	CFileMgr::ChangeDir("\\DATA\\MAPS\\");
	int fd = CFileMgr::OpenFile(filename, "r");
	assert(fd > 0);
	CFileMgr::ChangeDir("\\");
	for (line = CFileLoader::LoadLine(fd); line; line = CFileLoader::LoadLine(fd)) {
		if (*line == '\0' || *line == '#')
			continue;

		if (section == NONE) {
			if (isLine4(line, 'o','b','j','s')) section = OBJS;
			else if (isLine4(line, 't','o','b','j')) section = TOBJ;
			else if (isLine4(line, '2','d','f','x')) section = TWODFX;
		} else if (isLine3(line, 'e','n','d')) {
			section = NONE;
		} else {
			switch (section) {
				case OBJS:
				case TOBJ:
					ReloadObject(line);
					break;
				case TWODFX:
					Load2dEffect(line);
					break;
				default:
					break;
			}
		}
	}
	CFileMgr::CloseFile(fd);
}

void
CFileLoader::ReloadObject(const char *line)
{
	int id, numObjs;
	char model[24], txd[24];
	float dist[3];
	uint32 flags;
	CSimpleModelInfo *mi;

	if(sscanf(line, "%d %s %s %d", &id, model, txd, &numObjs) != 4)
		return;

	switch(numObjs){
	case 1:
		sscanf(line, "%d %s %s %d %f %d",
			&id, model, txd, &numObjs, &dist[0], &flags);
		break;
	case 2:
		sscanf(line, "%d %s %s %d %f %f %d",
			&id, model, txd, &numObjs, &dist[0], &dist[1], &flags);
		break;
	case 3:
		sscanf(line, "%d %s %s %d %f %f %f %d",
			&id, model, txd, &numObjs, &dist[0], &dist[1], &dist[2], &flags);
		break;
	}

	mi = (CSimpleModelInfo*) CModelInfo::GetModelInfo(id);
	if (
#ifdef FIX_BUGS
		mi &&
#endif
	    mi->GetModelType() == MITYPE_SIMPLE && !strcmp(mi->GetModelName(), model) && mi->m_numAtomics == numObjs) {
		mi->SetLodDistances(dist);
		SetModelInfoFlags(mi, flags);
	} else {
		printf("Can't reload %s\n", model);
	}
}

// unused mobile function - crashes
void
CFileLoader::ReLoadScene(const char *filename)
{
	char *line;
	CFileMgr::ChangeDir("\\DATA\\");
	int fd = CFileMgr::OpenFile(filename, "r");
	assert(fd > 0);
	CFileMgr::ChangeDir("\\");

	for (line = CFileLoader::LoadLine(fd); line; line = CFileLoader::LoadLine(fd)) {
		if (*line == '#')
			continue;

		if (strncmp(line, "EXIT", 4) == 0)
			break;

		if (strncmp(line, "IDE", 3) == 0) {
			LoadObjectTypes(line + 4);
		}
	}
	CFileMgr::CloseFile(fd);
}
