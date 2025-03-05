#include "common.h"

#include "AnimBlendAssociation.h"
#include "AnimBlendNode.h"

void 
CAnimBlendNode::Init(void)
{
	frameA = -1;
	frameB = -1;
	remainingTime = 0.0f;
	sequence = nil;
	association = nil;
}

void CAnimBlendNode::Setup() {
	player.Init(sequence->keyFrames, sequence->type, sequence->numFrames);
}

bool
CAnimBlendNode::Update(CVector &trans, CQuaternion &rot, float weight)
{
	assert (player.keyFrames == sequence->keyFrames);

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
		float kfAdt = player.GetDeltaTime(frameA);
		float t = kfAdt == 0.0f ? 0.0f : (kfAdt - remainingTime)/kfAdt;
		if(player.type & CAnimBlendSequence::KF_TRANS){
			auto kfAt = player.GetTranslation(frameA);
			auto kfBt = player.GetTranslation(frameB);
			trans = kfBt + t*(kfAt - kfBt);
			trans *= blend;
		}
		if(player.type & CAnimBlendSequence::KF_ROT){
			auto kfAr = player.GetRotation(frameA);
			auto kfBr = player.GetRotation(frameB);
			rot.Slerp(kfBr, kfAr, theta, invSin, t);
			rot *= blend;
		}
	}

	return looped;
}

// TODO: Check this
bool
CAnimBlendNode::NextKeyFrame(void)
{
	bool looped;

	if(player.numFrames <= 1)
		return false;

	looped = false;
	frameB = frameA;

	// Advance as long as we have to
	while(remainingTime <= 0.0f){
		frameA++;

		if(frameA >= player.numFrames){
			// reached end of animation
			if(!association->IsRepeating()){
				frameA--;
				frameB = frameA - 1;
				if(frameB < 0)
					frameB += player.numFrames;
				remainingTime = 0.0f;
				// // range chekcs
				// auto kfAt = player.GetTranslation(frameA);
				// auto kfBt = player.GetTranslation(frameB);
				return false;
			}
			looped = true;
			frameA = 0;
		}
		player.AdvanceFrame();
		remainingTime += player.GetDeltaTime(frameA);
	}

	frameB = frameA - 1;
	if(frameB < 0)
		frameB += player.numFrames;
	
	// // range chekcs
	// auto kfAt = player.GetTranslation(frameA);
	// auto kfBt = player.GetTranslation(frameB);

	CalcDeltas();
	return looped;
}

// Set animation to time t
bool
CAnimBlendNode::FindKeyFrame(float t)
{
	if(player.numFrames < 1)
		return false;

	frameA = 0;
	frameB = frameA;
	player.SeekToStart();

	assert (player.keyFrames == sequence->keyFrames);

	if(player.numFrames == 1){
		remainingTime = 0.0f;
	}else{
		// advance until t is between frameB and frameA
		frameA++;
		player.AdvanceFrame();
		while (t > player.GetDeltaTime(frameA)) {
			t -= player.GetDeltaTime(frameA);
			if (frameA + 1 >= player.numFrames) {
				// reached end of animation
				if (!association->IsRepeating()) {
					frameA --;
					// // range chekcs
					// auto kfAt = player.GetTranslation(frameA);
					// auto kfBt = player.GetTranslation(frameB);
					CalcDeltas();
					remainingTime = 0.0f;
					return false;
				}
				frameA = 0;
			}
			frameB = frameA;
			player.AdvanceFrame();
			frameA++;
		}

		remainingTime = player.GetDeltaTime(frameA) - t;
	}

	// // range chekcs
	// auto kfAt = player.GetTranslation(frameA);
	// auto kfBt = player.GetTranslation(frameB);

	CalcDeltas();
	return true;
}

void
CAnimBlendNode::CalcDeltas(void)
{
	if((player.type & CAnimBlendSequence::KF_ROT) == 0)
		return;
	auto kfAr = player.GetRotation(frameA);
	auto kfBr = player.GetRotation(frameB);
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
		auto kfAdt = player.GetDeltaTime(frameA);
		float t = kfAdt == 0.0f ? 0.0f : (kfAdt - remainingTime)/kfAdt;
		if(player.type & CAnimBlendSequence::KF_TRANS){
			auto kfAt = player.GetTranslation(frameA);
			auto kfBt = player.GetTranslation(frameB);
			trans = kfBt + t*(kfAt - kfBt);
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
		if(player.type & CAnimBlendSequence::KF_TRANS){
			CVector pos = sequence->GetEndTranslation();
			trans = pos * blend;
		}
	}
}