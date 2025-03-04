#pragma once

#include <cmath>

#include "Quaternion.h"

#ifdef MoveMemory
#undef MoveMemory	// windows shit
#endif

// TODO: put them somewhere else?
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
	int16 rot[4];		// 4096
	uint16 dltTime;	// 256

	CQuaternion rotation_() {
		return { rot[0] * (1/4096.f), rot[1] * (1/4096.f), rot[2] * (1/4096.f), rot[3] * (1/4096.f) };
	}

	void rotation_(const CQuaternion& q) {
		rot[0] = checked_f2i16(q.x * 4096.0f);
		rot[1] = checked_f2i16(q.y * 4096.0f);
		rot[2] = checked_f2i16(q.z * 4096.0f);
		rot[3] = checked_f2i16(q.w * 4096.0f);
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
	CVector trans;
	CVector translation_() {
		return trans;
	}

	void translation_(const CVector &v) {
		trans = v;
	}
};

struct KeyFrameTransCompressed : KeyFrame {
	int16 trans[3];		// 128

	CVector translation_() {
		return { trans[0] * (1/128.f), trans[1] * (1/128.f), trans[2] * (1/128.f)};
	}

	void translation_(const CVector &v) {
		trans[0] = checked_f2i16(v.x * 128.f);
		trans[1] = checked_f2i16(v.y * 128.f);
		trans[2] = checked_f2i16(v.z * 128.f);
	}
};

// The sequence of key frames of one animated node
class CAnimBlendSequence
{
	template <typename T>
	T get(unsigned offset) {
		return *(T*)((uint8_t*)keyFrames + offset);
	}

	template <typename T>
	T read() {
		T rv;
		memcpy(&rv, (uint8_t*)keyFrames + readOffset, sizeof(T));
		readOffset += sizeof(T);
		return rv;
	}

	CQuaternion fromSphericalFixed(uint16_t y, uint16_t p, uint16_t r) {
        CQuaternion q;
        q.w = cos((y / 65536.0f) * 2 * M_PI) * cos((p / 65536.0f) * 2 * M_PI);
        q.x = cos((y / 65536.0f) * 2 * M_PI) * sin((p / 65536.0f) * 2 * M_PI);
        q.y = sin((y / 65536.0f) * 2 * M_PI) * cos((r / 65536.0f) * 2 * M_PI);
        q.z = sin((y / 65536.0f) * 2 * M_PI) * sin((r / 65536.0f) * 2 * M_PI);
        return q;
    }
public:
	enum {
		KF_ROT = 1,
		KF_TRANS = 2,
		
		FLAGS_HAS_ROT_Y = 1 << 8,
		FLAGS_HAS_ROT_P = 1 << 9,
		FLAGS_HAS_ROT_R = 1 << 10,

		FLAGS_HAS_TRANS_X = 1 << 11,
		FLAGS_HAS_TRANS_Y = 1 << 12,
		FLAGS_HAS_TRANS_Z = 1 << 13,
		FLAGS_HAS_TRANS_ANY = 7 << 11,
		FLAGS_HAS_TRANS_LARGE = 1 << 14,
		FLAGS_QUAT0_NEG = 1 << 15
	};
	int32 type;
	char name[24];
	int32 numFrames;
	int32 curFrame;
	int16 boneTag;
	void *keyFrames;
	CQuaternion currentRotation;
	CQuaternion nextRotation;
	CVector currentTranslation;
	CVector nextTranslation;
	CVector startTranslation, endTranslation;
	float endTime;

	unsigned readOffset;

	uint16_t predicted_y, predicted_p, predicted_r;
	float predicted_tx = 0, predicted_ty = 0, predicted_tz = 0;
	float nextDeltaTime;

	void Init() {
		curFrame = 0;
		readOffset = 0;

		float startTime = read<float>();
		endTime = read<float>();

		if (type & KF_TRANS) {
			if (type & FLAGS_HAS_TRANS_LARGE) {
                startTranslation.x = read<float>();
                startTranslation.y = read<float>();
                startTranslation.z = read<float>();
                predicted_tx = startTranslation.x;
                predicted_ty = startTranslation.y;
                predicted_tz = startTranslation.z;

                // Read final translation (may be used for verification or ignored)
                endTranslation.x = read<float>();
                endTranslation.y = read<float>();
                endTranslation.z = read<float>();
            } else {
                startTranslation.x = read<int16_t>() / 128.f;
                startTranslation.y = read<int16_t>() / 128.f;
                startTranslation.z = read<int16_t>() / 128.f;
                predicted_tx = startTranslation.x;
                predicted_ty = startTranslation.y;
                predicted_tz = startTranslation.z;

                // Read final translation (for completeness)
                endTranslation.x = read<int16_t>() / 128.f;
                endTranslation.y = read<int16_t>() / 128.f;
                endTranslation.z = read<int16_t>() / 128.f;
            }

			nextTranslation = startTranslation;
		} else {
			startTranslation = { 0, 0, 0 };
			endTranslation = { 0, 0, 0 };
			nextTranslation = startTranslation;
		}

		predicted_y = read<uint16_t>();
        predicted_p = read<uint16_t>();
        predicted_r = read<uint16_t>();
		nextRotation = fromSphericalFixed(predicted_y, predicted_p, predicted_r);

		if (type & FLAGS_QUAT0_NEG) {
            nextRotation = -nextRotation;
        }

		if (numFrames > 1) {
			AdvanceFrame();
		} else {
			currentTranslation = nextTranslation;
			currentRotation = nextRotation;
			nextDeltaTime = 0;
		}
	}
	CQuaternion GetCurrentRotation() {
		return currentRotation;
	}
	CQuaternion GetNextRotation() {
		return nextRotation;
	}
	float GetNextTimeDelta() {
		return nextDeltaTime;
	}
	float GetEndTime() {
		return endTime;
	}
	CVector GetStartTranslation() {
		return startTranslation;
	}
	CVector GetCurrentTranslation() {
		return currentTranslation;
	}
	CVector GetNextTranslationDelta() {
		return nextTranslation - currentTranslation;
	}
	CVector GetEndTranslation() {
		return endTranslation;
	}

	void AdvanceFrame() {
		if (++curFrame == numFrames){
			Init();
		}

		// rotation
		{
			currentRotation = nextRotation;
			
			// For rotation Y:
			if (type & FLAGS_HAS_ROT_Y) {
				uint8_t byteVal = read<uint8_t>();
				if (byteVal == 128) {
					predicted_y = read<uint16_t>();
				} else {
					int8_t diff = static_cast<int8_t>(byteVal);
					predicted_y += diff * 8;
				}
			}
			// For rotation P:
			if (type & FLAGS_HAS_ROT_P) {
				uint8_t byteVal = read<uint8_t>();
				if (byteVal == 128) {
					predicted_p = read<uint16_t>();
				} else {
					int8_t diff = static_cast<int8_t>(byteVal);
					predicted_p += diff * 8;
				}
			}
			// For rotation R:
			if (type & FLAGS_HAS_ROT_R) {
				uint8_t byteVal = read<uint8_t>();
				if (byteVal == 128) {
					predicted_r = read<uint16_t>();
				} else {
					int8_t diff = static_cast<int8_t>(byteVal);
					predicted_r += diff * 8;
				}
			}

			nextRotation = fromSphericalFixed(predicted_y, predicted_p, predicted_r);
		}

		// translation
		if (type & KF_TRANS) {
			currentTranslation = nextTranslation;
			if (type & FLAGS_HAS_TRANS_X) {
                uint8_t byteVal = read<uint8_t>();
                if (byteVal == 128) {
                    uint16_t diff = read<uint16_t>();
                    if (diff != 32768) {
                        predicted_tx += static_cast<int16_t>(diff) / 128.f;
                    } else {
                        predicted_tx = read<float>();
                    }
                } else {
                    int8_t diff = static_cast<int8_t>(byteVal);
                    predicted_tx += diff / 127.f;
                }
            }
            // Translation Y:
            if (type & FLAGS_HAS_TRANS_Y) {
                uint8_t byteVal = read<uint8_t>();
                if (byteVal == 128) {
                    uint16_t diff = read<uint16_t>();
                    if (diff != 32768) {
                        predicted_ty += static_cast<int16_t>(diff) / 128.f;
                    } else {
                        predicted_ty = read<float>();
                    }
                } else {
                    int8_t diff = static_cast<int8_t>(byteVal);
                    predicted_ty += diff / 127.f;
                }
            }
            // Translation Z:
            if (type & FLAGS_HAS_TRANS_Z) {
                uint8_t byteVal = read<uint8_t>();
                if (byteVal == 128) {
                    uint16_t diff = read<uint16_t>();
                    if (diff != 32768) {
                        predicted_tz += static_cast<int16_t>(diff) / 128.f;
                    } else {
                        predicted_tz = read<float>();
                    }
                } else {
                    int8_t diff = static_cast<int8_t>(byteVal);
                    predicted_tz += diff / 127.f;
                }
            }
		
			nextTranslation = { predicted_tx, predicted_ty, predicted_tz };
		}
	
		// time delta + quaternion flips
		{
			uint8_t byteValPacked = read<uint8_t>();
			uint8_t byteVal = byteValPacked & 127;
			float diff;
			if (byteVal == 127) {
				uint16_t fixed_diff = read<uint16_t>();
				diff = fixed_diff / 256.f;
			} else {
				diff = byteVal / 256.f;
			}
			nextDeltaTime = diff;

			if (byteValPacked & 128) {
				nextRotation = -nextRotation;
			}
		}
	}

	CAnimBlendSequence(void);
	virtual ~CAnimBlendSequence(void);
	void SetName(char *name);

	bool HasTranslation(void) { return !!(type & KF_TRANS); }
	bool MoveMemory(void);

	void SetBoneTag(int tag) { boneTag = tag; }
};
VALIDATE_SIZE(CAnimBlendSequence, 0x30);
