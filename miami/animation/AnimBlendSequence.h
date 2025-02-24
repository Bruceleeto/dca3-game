#pragma once

#include "Quaternion.h"

#ifdef MoveMemory
#undef MoveMemory	// windows shit
#endif

// TODO: put them somewhere else?
static int16 checked_f2i8(float f) {
	assert(f >= -128 && f <= 127);
	return f;
}

static uint8 checked_f2u8(float f) {
	assert(f >= 0 && f <= 255);
	return f;
}

static int16 checked_f2i16(float f) {
	assert(f >= -32768 && f <= 32767);
	return f;
}

static uint16 checked_f2u16(float f) {
	assert(f >= 0 && f <= 65535);
	return f;
}

#define KF_MINDELTA (1/256.f)

struct KeyFrame {
	int8 rot[4];		// 127
	uint16 dltTime;	// 256

	CQuaternion rotation_() {
		return { rot[0] * (1/127.f), rot[1] * (1/127.f), rot[2] * (1/127.f), rot[3] * (1/127.f) };
	}

	void rotation_(const CQuaternion& q) {
		rot[0] = checked_f2i8(q.x * 127.0f);
		rot[1] = checked_f2i8(q.y * 127.0f);
		rot[2] = checked_f2i8(q.z * 127.0f);
		rot[3] = checked_f2i8(q.w * 127.0f);
	}

	float deltaTime_() {
		return dltTime * (1/256.0f);
	}

	void deltaTime_(float t) {
		dltTime = checked_f2u16(t * 256); // always round down
	}
};

struct KeyFrameTransUncompressed : KeyFrame {
	// Some animations use bigger range, eg during the intro
	// CVector trans;
	int8_t x, y, z;
	float scale;
	CVector translation_() {
		CVector n = { x * (1/127.f), y * (1/127.f), z * (1/127.f) };
		return n * scale;
	}

	void translation_(const CVector &v) {
		CVector n = v;
		n.Normalise();
		x = checked_f2i8(n.x * 127.0f);
		y = checked_f2i8(n.y * 127.0f);
		z = checked_f2i8(n.z * 127.0f);
		scale = v.Magnitude();
	}
};

struct KeyFrameTransCompressed : KeyFrame {
	int8 trans[3];		// 128

	CVector translation_() {
		return { trans[0] * (1.f), trans[1] * (1.f), trans[2] * (1.f)};
	}

	void translation_(const CVector &v) {
		trans[0] = checked_f2i8(v.x );
		trans[1] = checked_f2i8(v.y );
		trans[2] = checked_f2i8(v.z );
	}
};

// The sequence of key frames of one animated node
class CAnimBlendSequence
{
public:
	enum {
		KF_ROT = 1,
		KF_TRANS = 2,
		KF_COMPRESSED = 4, // only applicable for KF_TRANS
	};
	int32 type;
	char name[24];
	int32 numFrames;
	int16 boneTag;
	void *keyFrames;

	CAnimBlendSequence(void);
	virtual ~CAnimBlendSequence(void);
	void SetName(char *name);
	void SetNumFrames(int numFrames, bool translation, bool compressed);
	void RemoveQuaternionFlips(void);

	void SetTranslation(int n, const CVector &v) {
		if (type & KF_COMPRESSED) {
			((KeyFrameTransCompressed*)keyFrames)[n].translation_(v);
		} else if (type & KF_TRANS) {
			((KeyFrameTransUncompressed*)keyFrames)[n].translation_(v);
		} else {
			assert(false && "SetTranslation called on sequence without translation");
		}
	}

	CVector GetTranslation(int n) {
		if (type & KF_COMPRESSED) {
			return ((KeyFrameTransCompressed*)keyFrames)[n].translation_();
		} else if (type & KF_TRANS) {
			return ((KeyFrameTransUncompressed*)keyFrames)[n].translation_();
		} else {
			assert(false && "GetTranslation called on sequence without translation");
		}
	}

	void SetRotation(int n, const CQuaternion &q) {
		if (type & KF_COMPRESSED) {
			((KeyFrameTransCompressed*)keyFrames)[n].rotation_(q);
		} else if (type & KF_TRANS) {
			((KeyFrameTransUncompressed*)keyFrames)[n].rotation_(q);
		} else {
			((KeyFrame*)keyFrames)[n].rotation_(q);
		}
	}

	CQuaternion GetRotation(int n) {
		if (type & KF_COMPRESSED) {
			return ((KeyFrameTransCompressed*)keyFrames)[n].rotation_();
		} else if (type & KF_TRANS) {
			return ((KeyFrameTransUncompressed*)keyFrames)[n].rotation_();
		} else {
			return ((KeyFrame*)keyFrames)[n].rotation_();
		}
	}

	void SetDeltaTime(int n, float t) {
		if (type & KF_COMPRESSED) {
			((KeyFrameTransCompressed*)keyFrames)[n].deltaTime_(t);
		} else if (type & KF_TRANS) {
			((KeyFrameTransUncompressed*)keyFrames)[n].deltaTime_(t);
		} else {
			((KeyFrame*)keyFrames)[n].deltaTime_(t);
		}
	}

	float GetDeltaTime(int n) {
		if (type & KF_COMPRESSED) {
			return ((KeyFrameTransCompressed*)keyFrames)[n].deltaTime_();
		} else if (type & KF_TRANS) {
			return ((KeyFrameTransUncompressed*)keyFrames)[n].deltaTime_();
		} else {
			return ((KeyFrame*)keyFrames)[n].deltaTime_();
		}
	}

	bool HasTranslation(void) { return !!(type & KF_TRANS); }
	bool MoveMemory(void);

	void SetBoneTag(int tag) { boneTag = tag; }
};
VALIDATE_SIZE(CAnimBlendSequence, 0x30);
