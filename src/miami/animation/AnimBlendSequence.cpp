#include "common.h"

#include "AnimBlendSequence.h"
#include "MemoryHeap.h"

void* obj_alloc(size_t size, void** storage);
void obj_free(void* ptr);
void* obj_move(void* ptr);

CAnimBlendSequence::CAnimBlendSequence(void)
{
	type = 0;
	numFrames = 0;
	keyFrames = nil;
	boneTag = -1;
}

CAnimBlendSequence::~CAnimBlendSequence(void)
{
	if(keyFrames)
		obj_free(keyFrames);
}

void
CAnimBlendSequence::SetName(char *name)
{
	strncpy(this->name, name, 24);
}

#ifdef USE_CUSTOM_ALLOCATOR
bool
CAnimBlendSequence::MoveMemory(void)
{
	if(keyFrames){
		void *newaddr = gMainHeap.MoveMemory(keyFrames);
		if(newaddr != keyFrames){
			keyFrames = newaddr;
			return true;
		}
	}else if(keyFramesCompressed){
		void *newaddr = gMainHeap.MoveMemory(keyFramesCompressed);
		if(newaddr != keyFramesCompressed){
			keyFramesCompressed = newaddr;
			return true;
		}
	}
	return false;
}
#endif

