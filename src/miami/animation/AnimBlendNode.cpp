#include "common.h"

#include "AnimBlendAssociation.h"
#include "AnimBlendNode.h"

void 
CAnimBlendNode::Init(void)
{
	frameA = -1;
	remainingTime = 0.0f;
	sequence = nil;
	association = nil;
	player = nil;
}

void CAnimBlendNode::Destroy(void) {
	if (player) {
		delete player;
		player = nil;
	}
}

bool
CAnimBlendNode::Update(CVector &trans, CQuaternion &rot, float weight)
{
	assert (player);

	bool looped = false;

	trans = CVector(0.0f, 0.0f, 0.0f);
	rot = CQuaternion(0.0f, 0.0f, 0.0f, 0.0f);

	if(association->IsRunning()){
		remainingTime -= association->timeStep;
		if(remainingTime <= 0.0f)
			looped = NextKeyFrame();
	}

	float blend = association->GetBlendAmount(weight);
	if(blend > 0.0f){
		float kfAdt = player->GetNextTimeDelta();
		float t = kfAdt == 0.0f ? 0.0f : (kfAdt - remainingTime)/kfAdt;
		if(player->type & CAnimBlendSequence::KF_TRANS){
			auto kfdAt = player->GetNextTranslationDelta();
			auto kfBt = player->GetPrevTranslation();
			trans = kfBt + t*kfdAt;
			trans *= blend;
		}
		if(player->type & CAnimBlendSequence::KF_ROT){
			auto kfAr = player->GetNextRotation();
			auto kfBr = player->GetPrevRotation();
			rot.Slerp(kfBr, kfAr, theta, invSin, t);
			rot *= blend;
		}
	}

	return looped;
}

bool
CAnimBlendNode::NextKeyFrame(void)
{
	assert(player != nil);
	bool looped;

	if(player->numFrames <= 1)
		return false;

	looped = false;

	// Advance as long as we have to
	while(remainingTime <= 0.0f){
		frameA++;

		if(frameA >= player->numFrames){
			// reached end of animation
			if(!association->IsRepeating()){
				frameA--;
				remainingTime = 0.0f;
				assert(frameA == player->curFrame);
				CalcDeltas();
				return false;
			}
			looped = true;
			frameA = 0;
		}
		player->AdvanceFrame(sequence->keyFrames);
		remainingTime += player->GetNextTimeDelta();
	}

	assert(frameA == player->curFrame);
	CalcDeltas();
	return looped;
}

// Set animation to time t
bool
CAnimBlendNode::FindKeyFrame(float t)
{
	if (player == nil) {
		player = new CAnimBlendPlayer();
		player->Init(sequence->keyFrames, sequence->type, sequence->numFrames);
	}
	if(player->numFrames < 1)
		return false;

	frameA = 0;
	player->SeekToStart(sequence->keyFrames);

	if(player->numFrames == 1){
		remainingTime = 0.0f;
	}else{
		// advance until t is between frameB and frameA
		frameA++;
		player->AdvanceFrame(sequence->keyFrames);
		while (t > player->GetNextTimeDelta()) {
			t -= player->GetNextTimeDelta();
			if (frameA + 1 >= player->numFrames) {
				// reached end of animation
				if (!association->IsRepeating()) {
					assert(frameA == player->curFrame);
					CalcDeltas();
					remainingTime = 0.0f;
					return false;
				}
				// Frame 0 is effectively skipped here
				// Looks like an re3 / game bug?
				frameA = 0;
				player->SeekToStart(sequence->keyFrames);
			}
			frameA++;
			player->AdvanceFrame(sequence->keyFrames);
		}

		remainingTime = player->GetNextTimeDelta() - t;
	}

	assert(frameA == player->curFrame);
	CalcDeltas();
	return true;
}

void
CAnimBlendNode::CalcDeltas(void)
{
	if((player->type & CAnimBlendSequence::KF_ROT) == 0)
		return;
	auto kfAr = player->GetNextRotation();
	auto kfBr = player->GetPrevRotation();
	float cos = DotProduct(kfAr, kfBr);
	if(cos > 1.0f)
		cos = 1.0f;
	theta = Acos(cos);
	invSin = theta == 0.0f ?  0.0f : 1.0f/Sin(theta);
}

void
CAnimBlendNode::GetCurrentTranslation(CVector &trans, float weight)
{
	trans = CVector(0.0f, 0.0f, 0.0f);

	float blend = association->GetBlendAmount(weight);
	if(blend > 0.0f){
		auto kfAdt = player->GetNextTimeDelta();
		float t = kfAdt == 0.0f ? 0.0f : (kfAdt - remainingTime)/kfAdt;
		if(player->type & CAnimBlendSequence::KF_TRANS){
			auto kfdAt = player->GetNextTranslationDelta();
			auto kfBt = player->GetPrevTranslation();
			trans = kfBt + t*kfdAt;
			trans *= blend;
		}
	}
}


void
CAnimBlendNode::GetEndTranslation(CVector &trans, float weight)
{
	trans = CVector(0.0f, 0.0f, 0.0f);

	float blend = association->GetBlendAmount(weight);
	if(blend > 0.0f){
		if(player->type & CAnimBlendSequence::KF_TRANS){
			CVector pos = sequence->GetEndTranslation();
			trans = pos * blend;
		}
	}
}