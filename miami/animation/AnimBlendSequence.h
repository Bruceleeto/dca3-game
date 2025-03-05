#pragma once

#include <cmath>

#include "Quaternion.h"

#ifdef MoveMemory
#undef MoveMemory	// windows shit
#endif

struct CAnimBlendPlayer {
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
	void* keyFrames;
	int32 curFrame;
	int32 numFrames;
	CQuaternion currentRotation;
	CQuaternion nextRotation;
	CVector currentTranslation;
	CVector nextTranslation;

	unsigned readOffset;
	unsigned readOffset_initial;

	uint16_t predicted_y, predicted_p, predicted_r;
	float predicted_tx = 0, predicted_ty = 0, predicted_tz = 0;
	float nextDeltaTime;

	#if !defined(DC_TEXCONV)
	static unsigned count;

	CAnimBlendPlayer() {
		count++;
		fprintf(stderr, "CAnimBlendPlayer count %d\n", count);
	}
	~CAnimBlendPlayer() {
		count--;
	}
	#endif
	
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

	void AdvanceFrame() {
		if (++curFrame == numFrames){
			currentRotation = nextRotation;
			currentTranslation = nextTranslation;
			SeekToStart();
			return;
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

	CQuaternion GetRotation(unsigned frame) {
		auto lastFrame = curFrame == 0 ? numFrames - 1 : curFrame - 1;
		if (frame == lastFrame) {
			return currentRotation;
		} else if (frame == curFrame) {
			return nextRotation;
		} else {
			assert(false);
		}
	}
	CVector GetTranslation(unsigned frame) {
		auto lastFrame = curFrame == 0 ? numFrames - 1 : curFrame - 1;
		if (frame == lastFrame) {
			return currentTranslation;
		} else if (frame == curFrame) {
			return nextTranslation;
		} else {
			assert(false);
		}
	}
	float GetDeltaTime(unsigned frame) {
		if (frame == curFrame) {
			return nextDeltaTime;
		} else {
			assert(false);
		}
		return 1/30.f;
	}

	// CQuaternion GetCurrentRotation() {
	// 	return currentRotation;
	// }
	// CQuaternion GetNextRotation() {
	// 	return nextRotation;
	// }
	// float GetNextTimeDelta() {
	// 	return nextDeltaTime;
	// }

	// CVector GetCurrentTranslation() {
	// 	return currentTranslation;
	// }
	// CVector GetNextTranslationDelta() {
	// 	return nextTranslation - currentTranslation;
	// }

	void Init(void* kf, int32 tp, int nF) {
		keyFrames = kf;
		type = tp;
		numFrames = nF;

		SeekToStart();
		currentTranslation = nextTranslation;
		currentRotation = nextRotation;
	}

	void SeekToStart() {
		readOffset = 0;
		float startTime = read<float>();
		float endTime = read<float>();

		if (type & KF_TRANS) {
			CVector startTranslation;
			if (type & FLAGS_HAS_TRANS_LARGE) {
				startTranslation.x = read<float>();
				startTranslation.y = read<float>();
				startTranslation.z = read<float>();
				predicted_tx = startTranslation.x;
				predicted_ty = startTranslation.y;
				predicted_tz = startTranslation.z;

				CVector endTranslation;
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

				CVector endTranslation;
				// Read final translation (for completeness)
				endTranslation.x = read<int16_t>() / 128.f;
				endTranslation.y = read<int16_t>() / 128.f;
				endTranslation.z = read<int16_t>() / 128.f;
			}

			nextTranslation = startTranslation;
		} else {
			CVector startTranslation = { 0, 0, 0 };
			CVector endTranslation = { 0, 0, 0 };
			nextTranslation = startTranslation;
		}

		predicted_y = read<uint16_t>();
		predicted_p = read<uint16_t>();
		predicted_r = read<uint16_t>();
		nextRotation = fromSphericalFixed(predicted_y, predicted_p, predicted_r);

		if (type & FLAGS_QUAT0_NEG) {
			nextRotation = -nextRotation;
		}

		nextDeltaTime = startTime;
		curFrame = 0;
	}
};

// The sequence of key frames of one animated node
class CAnimBlendSequence
{
	template <typename T>
	inline T read(uint32_t &readOffset) {
		T rv;
		memcpy(&rv, (uint8_t*)keyFrames + readOffset, sizeof(T));
		readOffset += sizeof(T);
		return rv;
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
	int16 boneTag;
	void *keyFrames;

	struct InitData {
		CVector startTranslation, endTranslation;
		float endTime;
	};

	inline InitData GetInitData() {
		InitData rv;
		uint32_t readOffset = 0;

		float startTime = read<float>(readOffset);
		rv.endTime = read<float>(readOffset);

		if (type & KF_TRANS) {
			if (type & FLAGS_HAS_TRANS_LARGE) {
                rv.startTranslation.x = read<float>(readOffset);
                rv.startTranslation.y = read<float>(readOffset);
                rv.startTranslation.z = read<float>(readOffset);

                // Read final translation (may be used for verification or ignored)
                rv.endTranslation.x = read<float>(readOffset);
                rv.endTranslation.y = read<float>(readOffset);
                rv.endTranslation.z = read<float>(readOffset);
            } else {
                rv.startTranslation.x = read<int16_t>(readOffset) / 128.f;
                rv.startTranslation.y = read<int16_t>(readOffset) / 128.f;
                rv.startTranslation.z = read<int16_t>(readOffset) / 128.f;

                // Read final translation (for completeness)
                rv.endTranslation.x = read<int16_t>(readOffset) / 128.f;
                rv.endTranslation.y = read<int16_t>(readOffset) / 128.f;
                rv.endTranslation.z = read<int16_t>(readOffset) / 128.f;
            }
		} else {
			rv.startTranslation = { 0, 0, 0 };
			rv.endTranslation = { 0, 0, 0 };
		}

		return rv;
	}


	CVector GetStartTranslation() {
		return GetInitData().startTranslation;
	}
	float GetEndTime() {
		return GetInitData().endTime;
	}
	CVector GetEndTranslation() {
		return GetInitData().endTranslation;
	}

	CAnimBlendSequence(void);
	virtual ~CAnimBlendSequence(void);
	void SetName(char *name);

	bool HasTranslation(void) { return !!(type & KF_TRANS); }
	bool MoveMemory(void);

	void SetBoneTag(int tag) { boneTag = tag; }
};
VALIDATE_SIZE(CAnimBlendSequence, 0x30);
