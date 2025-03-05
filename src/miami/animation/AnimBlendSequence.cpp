#include "common.h"

#include "AnimBlendSequence.h"
#include "MemoryHeap.h"

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
		RwFree(keyFrames);
}

void
CAnimBlendSequence::SetName(char *name)
{
	strncpy(this->name, name, 24);
}

// void
// CAnimBlendSequence::RemoveQuaternionFlips(void)
// {
// 	int i;
// 	CQuaternion last;

// 	if(numFrames < 2)
// 		return;

// 	last = GetRotation(0);
// 	for(i = 1; i < numFrames; i++){
// 		auto KFr = GetRotation(i);
// 		if(DotProduct(last, KFr) < 0.0f)
// 			SetRotation(i, -KFr);
// 		last = GetRotation(i);
// 	}
// }

// unsigned CAnimBlendPlayer::count = 0;

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

