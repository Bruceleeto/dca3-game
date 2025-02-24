#include "common.h"
#include "ColModel.h"
#include "Collision.h"
#include "Game.h"
#include "MemoryHeap.h"
#include "Pools.h"

void* obj_alloc(size_t size, void** storage);
void obj_free(void* ptr);
void* obj_move(void* ptr);

CColModel::CColModel(void)
{
	numSpheres = 0;
	spheres = nil;
	numLines = 0;
	lines = nil;
	numBoxes = 0;
	boxes = nil;
	numTriangles = 0;
	vertices = nil;
	triangles = nil;
	trianglePlanes = nil;
	level = LEVEL_GENERIC;	// generic col slot
	ownsCollisionVolumes = true;
}

CColModel::~CColModel(void)
{
	RemoveCollisionVolumes();
}

void*
CColModel::operator new(size_t) throw()
{
	CColModel* node = CPools::GetColModelPool()->New();
	assert(node);
	return node;
}

void
CColModel::operator delete(void *p, size_t) throw()
{
	CPools::GetColModelPool()->Delete((CColModel*)p);
}

void
CColModel::RemoveCollisionVolumes(void)
{
	if(ownsCollisionVolumes){
		obj_free(spheres);
		obj_free(lines);
		obj_free(boxes);
		obj_free(vertices);
		obj_free(triangles);
		CCollision::RemoveTrianglePlanes(this);
	}
	numSpheres = 0;
	numLines = 0;
	numBoxes = 0;
	numTriangles = 0;
	spheres = nil;
	lines = nil;
	boxes = nil;
	vertices = nil;
	triangles = nil;
}

void
CColModel::CalculateTrianglePlanes(void)
{
	PUSH_MEMID(MEMID_COLLISION);

	// HACK: allocate space for one more element to stuff the link pointer into
	trianglePlanes = (CColTrianglePlane*)obj_alloc(sizeof(CColTrianglePlane) * (numTriangles+1), (void**)&trianglePlanes);
	REGISTER_MEMPTR(&trianglePlanes);
	for(int i = 0; i < numTriangles; i++)
		trianglePlanes[i].Set(vertices, triangles[i]);

	POP_MEMID();
}

void
CColModel::RemoveTrianglePlanes(void)
{
	obj_free(trianglePlanes);
	trianglePlanes = nil;
}

void
CColModel::SetLinkPtr(CLink<CColModel*> *lptr)
{
	assert(trianglePlanes);
	*(CLink<CColModel*>**)ALIGNPTR(&trianglePlanes[numTriangles]) = lptr;
}

CLink<CColModel*>*
CColModel::GetLinkPtr(void)
{
	assert(trianglePlanes);
	return *(CLink<CColModel*>**)ALIGNPTR(&trianglePlanes[numTriangles]);
}

void
CColModel::GetTrianglePoint(CVector &v, int i) const
{
	v = vertices[i].Get();
}

CColModel&
CColModel::operator=(const CColModel &other)
{
	int i;
	int numVerts;

	boundingSphere = other.boundingSphere;
	boundingBox = other.boundingBox;

	// copy spheres
	if(other.numSpheres){
		if(numSpheres != other.numSpheres){
			numSpheres = other.numSpheres;
			if(spheres)
				obj_free(spheres);
			spheres = (CColSphere*)obj_alloc(numSpheres*sizeof(CColSphere), (void**)&spheres);
		}
		for(i = 0; i < numSpheres; i++)
			spheres[i] = other.spheres[i];
	}else{
		numSpheres = 0;
		if(spheres)
		obj_free(spheres);
		spheres = nil;
	}

	// copy lines
	if(other.numLines){
		if(numLines != other.numLines){
			numLines = other.numLines;
			if(lines)
			obj_free(lines);
			lines = (CColLine*)obj_alloc(numLines*sizeof(CColLine), (void**)&lines);
		}
		for(i = 0; i < numLines; i++)
			lines[i] = other.lines[i];
	}else{
		numLines = 0;
		if(lines)
			obj_free(lines);
		lines = nil;
	}

	// copy boxes
	if(other.numBoxes){
		if(numBoxes != other.numBoxes){
			numBoxes = other.numBoxes;
			if(boxes)
				obj_free(boxes);
			boxes = (CColBox*)obj_alloc(numBoxes*sizeof(CColBox), (void**)&boxes);
		}
		for(i = 0; i < numBoxes; i++)
			boxes[i] = other.boxes[i];
	}else{
		numBoxes = 0;
		if(boxes)
			obj_free(boxes);
		boxes = nil;
	}

	// copy mesh
	if(other.numTriangles){
		// copy vertices
		numVerts = 0;
		for(i = 0; i < other.numTriangles; i++){
			if(other.triangles[i].a > numVerts)
				numVerts = other.triangles[i].a;
			if(other.triangles[i].b > numVerts)
				numVerts = other.triangles[i].b;
			if(other.triangles[i].c > numVerts)
				numVerts = other.triangles[i].c;
		}
		numVerts++;
		if(vertices)
			obj_free(vertices);
		if(numVerts){
			vertices = (CompressedVector*)obj_alloc(numVerts*sizeof(CompressedVector), (void**)&vertices);
			for(i = 0; i < numVerts; i++)
				vertices[i] = other.vertices[i];
		}

		// copy triangles
		if(numTriangles != other.numTriangles){
			numTriangles = other.numTriangles;
			if(triangles)
				obj_free(triangles);
			triangles = (CColTriangle*)obj_alloc(numTriangles*sizeof(CColTriangle), (void**)&triangles);
		}
		for(i = 0; i < numTriangles; i++)
			triangles[i] = other.triangles[i];
	}else{
		numTriangles = 0;
		if(triangles)
			obj_free(triangles);
		triangles = nil;
		if(vertices)
			obj_free(vertices);
		vertices = nil;
	}
	return *this;
}
