#pragma once

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
	// static unsigned count;

	// CAnimBlendPlayer() {
	// 	count++;
	// 	fprintf(stderr, "CAnimBlendPlayer count %d\n", count);
	// }
	// ~CAnimBlendPlayer() {
	// 	count--;
	// }
	#endif
	
	template <typename T>
	T read_unaligned(uint32_t ro) {
		T rv;
		for (unsigned i = 0; i < sizeof(T); i++) {
			((uint8_t*)&rv)[i] = ((uint8_t*)keyFrames)[ro];
			ro++;
		}
		readOffset = ro;
		return rv;
	}
	template <typename T>
	__always_inline T read() {
		if (!(readOffset & (sizeof(T) -1))) {
			return read_aligned<T>();
		} else {
			return read_unaligned<T>(readOffset);
		}
	}

	template <typename T>
	__always_inline T read_aligned() {
		T rv;
		rv = *(T*)((uint8_t*)keyFrames + readOffset);
		readOffset += sizeof(T);
		return rv;
	}

	__always_inline CQuaternion fromSphericalFixed(uint16_t y, uint16_t p, uint16_t r) {
		CQuaternion q;
		#if !defined(DC_SH4)
			q.w = cos((y / 65536.0f) * 2 * M_PI) * cos((p / 65536.0f) * 2 * M_PI);
			q.x = cos((y / 65536.0f) * 2 * M_PI) * sin((p / 65536.0f) * 2 * M_PI);
			q.y = sin((y / 65536.0f) * 2 * M_PI) * cos((r / 65536.0f) * 2 * M_PI);
			q.z = sin((y / 65536.0f) * 2 * M_PI) * sin((r / 65536.0f) * 2 * M_PI);
		#else
			register float __ys __asm__("fr0");
			register float __yc __asm__("fr1");
			register float __ps __asm__("fr2");
			register float __pc __asm__("fr3");
			register float __rs __asm__("fr4");
			register float __rc __asm__("fr5");

			__asm__ __volatile__( 
				R"(
					lds	%[y],fpul
					fsca fpul, dr0
					lds	%[p],fpul
					fsca fpul, dr2
					lds	%[r],fpul
					fsca fpul, dr4
				)"
				: "=f" (__ys), "=f" (__yc), "=f" (__ps), "=f" (__pc), "=f" (__rs), "=f" (__rc)
				: "0" (__ys), "1" (__yc), "2" (__ps), "3" (__pc), "4" (__rs), "5" (__rc), [y]"r"(y), [p]"r"(p), [r]"r"(r));
			
			q.w = __yc * __pc;
			q.x = __yc * __ps;
			q.y = __ys * __rc;
			q.z = __ys * __rs;
		#endif
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
		float startTime = read_aligned<float>();
		float endTime = read_aligned<float>();

		if (type & KF_TRANS) {
			CVector startTranslation;
			if (type & FLAGS_HAS_TRANS_LARGE) {
				startTranslation.x = read_aligned<float>();
				startTranslation.y = read_aligned<float>();
				startTranslation.z = read_aligned<float>();
				predicted_tx = startTranslation.x;
				predicted_ty = startTranslation.y;
				predicted_tz = startTranslation.z;

				CVector endTranslation;
				// Read final translation (may be used for verification or ignored)
				endTranslation.x = read_aligned<float>();
				endTranslation.y = read_aligned<float>();
				endTranslation.z = read_aligned<float>();
			} else {
				startTranslation.x = read_aligned<int16_t>() / 128.f;
				startTranslation.y = read_aligned<int16_t>() / 128.f;
				startTranslation.z = read_aligned<int16_t>() / 128.f;
				predicted_tx = startTranslation.x;
				predicted_ty = startTranslation.y;
				predicted_tz = startTranslation.z;

				CVector endTranslation;
				// Read final translation (for completeness)
				endTranslation.x = read_aligned<int16_t>() / 128.f;
				endTranslation.y = read_aligned<int16_t>() / 128.f;
				endTranslation.z = read_aligned<int16_t>() / 128.f;
			}

			nextTranslation = startTranslation;
		} else {
			CVector startTranslation = { 0, 0, 0 };
			CVector endTranslation = { 0, 0, 0 };
			nextTranslation = startTranslation;
		}

		predicted_y = read_aligned<uint16_t>();
		predicted_p = read_aligned<uint16_t>();
		predicted_r = read_aligned<uint16_t>();
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
	__always_inline T read_aligned(uint32_t &readOffset) {
		T rv;
		rv = *(T*)((uint8_t*)keyFrames + readOffset);
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
#ifdef PED_SKIN
	int16 boneTag;
#endif
	void *keyFrames;


	struct InitData {
		CVector startTranslation, endTranslation;
		float endTime;
	};

	__always_inline InitData GetInitData() {
		InitData rv;
		uint32_t readOffset = 0;

		float startTime = read_aligned<float>(readOffset);
		rv.endTime = read_aligned<float>(readOffset);

		if (type & KF_TRANS) {
			if (type & FLAGS_HAS_TRANS_LARGE) {
                rv.startTranslation.x = read_aligned<float>(readOffset);
                rv.startTranslation.y = read_aligned<float>(readOffset);
                rv.startTranslation.z = read_aligned<float>(readOffset);

                // Read final translation (may be used for verification or ignored)
                rv.endTranslation.x = read_aligned<float>(readOffset);
                rv.endTranslation.y = read_aligned<float>(readOffset);
                rv.endTranslation.z = read_aligned<float>(readOffset);
            } else {
                rv.startTranslation.x = read_aligned<int16_t>(readOffset) / 128.f;
                rv.startTranslation.y = read_aligned<int16_t>(readOffset) / 128.f;
                rv.startTranslation.z = read_aligned<int16_t>(readOffset) / 128.f;

                // Read final translation (for completeness)
                rv.endTranslation.x = read_aligned<int16_t>(readOffset) / 128.f;
                rv.endTranslation.y = read_aligned<int16_t>(readOffset) / 128.f;
                rv.endTranslation.z = read_aligned<int16_t>(readOffset) / 128.f;
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

#ifdef PED_SKIN
	void SetBoneTag(int tag) { boneTag = tag; }
#endif
};
#ifndef PED_SKIN
VALIDATE_SIZE(CAnimBlendSequence, 0x2C);
#endif
