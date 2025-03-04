#include "common.h"

#include "AnimBlendAssociation.h"
#include "AnimBlendNode.h"

void 
CAnimBlendNode::Init(void)
{
	_frameA = -1;
	_frameB = -1;
	remainingTime = 0.0f;
	sequence = nil;
	association = nil;
}

bool
CAnimBlendNode::Update(CVector &trans, CQuaternion &rot, float weight)
{
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
		float kfAdt = sequence->GetNextTimeDelta();
		float t = kfAdt == 0.0f ? 0.0f : (kfAdt - remainingTime)/kfAdt;
		if(sequence->type & CAnimBlendSequence::KF_TRANS){
			auto kfdAt = sequence->GetNextTranslationDelta();
			auto kfBt = sequence->GetCurrentTranslation();
			trans = kfBt + t*kfdAt;
			trans *= blend;
		}
		if(sequence->type & CAnimBlendSequence::KF_ROT){
			auto kfAr = sequence->GetNextRotation();
			auto kfBr = sequence->GetCurrentRotation();
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

	if(sequence->numFrames <= 1)
		return false;

	looped = false;
	_frameB = _frameA;

	// Advance as long as we have to
	while(remainingTime <= 0.0f){
		_frameA++;

		if(_frameA >= sequence->numFrames){
			// reached end of animation
			if(!association->IsRepeating()){
				_frameA--;
				remainingTime = 0.0f;
				return false;
			}
			looped = true;
			_frameA = 0;
		}
		sequence->AdvanceFrame();
		remainingTime += sequence->GetNextTimeDelta();
	}

	_frameB = _frameA - 1;
	if(_frameB < 0)
		_frameB += sequence->numFrames;

	CalcDeltas();
	return looped;
}

// bool
// CAnimBlendNode::NextKeyFrameCompressed(void)
// {
// 	bool looped;

// 	if(sequence->numFrames <= 1)
// 		return false;

// 	looped = false;
// 	frameB = frameA;

// 	// Advance as long as we have to
// 	while(remainingTime <= 0.0f){
// 		frameA++;

// 		if(frameA >= sequence->numFrames){
// 			// reached end of animation
// 			if(!association->IsRepeating()){
// 				frameA--;
// 				remainingTime = 0.0f;
// 				return false;
// 			}
// 			looped = true;
// 			frameA = 0;
// 		}

// 		remainingTime += sequence->GetKeyFrameCompressed(frameA)->GetDeltaTime();
// 	}

// 	frameB = frameA - 1;
// 	if(frameB < 0)
// 		frameB += sequence->numFrames;

// 	CalcDeltasCompressed();
// 	return looped;
// }

// Set animation to time t
bool
CAnimBlendNode::FindKeyFrame(float t)
{
	if(sequence->numFrames < 1)
		return false;

	_frameA = 0;
	_frameB = _frameA;

	if(sequence->numFrames == 1){
		remainingTime = 0.0f;
	}else{
		// advance until t is between frameB and frameA
		sequence->AdvanceFrame();
		_frameA++;
		while (t > sequence->GetNextTimeDelta()) {
			t -= sequence->GetNextTimeDelta();
			if (_frameA + 1 >= sequence->numFrames) {
				// reached end of animation
				if (!association->IsRepeating()) {
					CalcDeltas();
					remainingTime = 0.0f;
					return false;
				}
				_frameA = 0;
			}
			_frameB = _frameA;

			sequence->AdvanceFrame();
			_frameA++;
		}

		remainingTime = sequence->GetNextTimeDelta() - t;
	}

	CalcDeltas();
	return true;
}

void
CAnimBlendNode::CalcDeltas(void)
{
	if((sequence->type & CAnimBlendSequence::KF_ROT) == 0)
		return;
	auto kfAr = sequence->GetNextRotation();
	auto kfBr = sequence->GetCurrentRotation();
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
		auto kfAdt = sequence->GetNextTimeDelta();
		float t = kfAdt == 0.0f ? 0.0f : (kfAdt - remainingTime)/kfAdt;
		if(sequence->type & CAnimBlendSequence::KF_TRANS){
			auto kfdAt = sequence->GetNextTranslationDelta();
			auto kfBt = sequence->GetCurrentTranslation();
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
		if(sequence->type & CAnimBlendSequence::KF_TRANS){
			CVector pos = sequence->GetEndTranslation();
			trans = pos * blend;
		}
	}
}