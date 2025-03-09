#include "common.h"

#include "AnimBlendSequence.h"
#include "AnimBlendHierarchy.h"
#include "AnimManager.h"

CAnimBlendHierarchy::CAnimBlendHierarchy(void)
{
	sequences = nil;
	numSequences = 0;
	totalLength = 0.0f;
}

void
CAnimBlendHierarchy::Shutdown(void)
{
	RemoveAnimSequences();
	totalLength = 0.0f;
}

void
CAnimBlendHierarchy::SetName(char *name)
{
	strncpy(this->name, name, 24);
}

void
CAnimBlendHierarchy::CalcTotalTime(void)
{
	int i, j;
	totalLength = 0.0f;

	for(i = 0; i < numSequences; i++){
#ifdef FIX_BUGS
		if(sequences[i].numFrames == 0)
			continue;
#endif
		float seqTime = 0.0f;
		for(j = 0; j < sequences[i].numFrames; j++)
			seqTime += sequences[i].GetDeltaTime(j);
		totalLength = Max(totalLength, seqTime);
	}
}

void
CAnimBlendHierarchy::RemoveQuaternionFlips(void)
{
	int i;

	for(i = 0; i < numSequences; i++)
		sequences[i].RemoveQuaternionFlips();
}

void
CAnimBlendHierarchy::RemoveAnimSequences(void)
{
	delete[] sequences;
	sequences = nil;
	numSequences = 0;
}


#ifdef USE_CUSTOM_ALLOCATOR
void
CAnimBlendHierarchy::MoveMemory(bool onlyone)
{
	int i;
	for(i = 0; i < numSequences; i++)
		if(sequences[i].MoveMemory() && onlyone)
			return;
}
#endif
