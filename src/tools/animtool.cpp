#include <cstdio>
#include <cstddef>
#include <cassert>
#include <cstring>
#include <vector>
#include <iostream>
#include <cstdint>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

struct Writer: std::vector<uint8_t> {
    template<typename T>
    void write(float v);
};


template<>
void Writer::write<int8_t>(float v) {
    assert(v >= -127 && v <= 127);
    push_back(static_cast<int8_t>(v));
}

template<>
void Writer::write<uint8_t>(float v) {
    assert(v >= 0 && v <= 255);
    push_back(static_cast<uint8_t>(v));
}

template<>
void Writer::write<int16_t>(float v) {
    assert(v >= -32767 && v <= 32767);
    int16_t fx = static_cast<int16_t>(v);
    insert(end(), reinterpret_cast<uint8_t*>(&fx), reinterpret_cast<uint8_t*>(&fx) + sizeof(fx));
}

template<>
void Writer::write<uint16_t>(float v) {
    assert(v >= 0 && v <= 65535);
    int16_t fx = static_cast<uint16_t>(v);
    insert(end(), reinterpret_cast<uint8_t*>(&fx), reinterpret_cast<uint8_t*>(&fx) + sizeof(fx));
}

template<>
void Writer::write<float>(float v) {
    insert(end(), reinterpret_cast<uint8_t*>(&v), reinterpret_cast<uint8_t*>(&v) + sizeof(float));
}

// Example Reader class interface (you need to implement this as appropriate)
class Reader {
public:
    Reader(const std::vector<uint8_t>& data) : buffer(data), offset(0) {}

    template<typename T>
    T read() {
        if (offset + sizeof(T) > buffer.size()) {
            fprintf(stderr, "Reader error: trying to read past end of buffer.\n");
            assert(false);
        }
        T value;
        memcpy(&value, buffer.data() + offset, sizeof(T));
        offset += sizeof(T);
        return value;
    }
private:
    const std::vector<uint8_t>& buffer;
    size_t offset;
};

#define FLAGS_KF_ROT ( 1 << 0 )
#define FLAGS_KF_TRANS ( 1 << 1 )

#define FLAGS_HAS_ROT_Y ( 1 << 8 )
#define FLAGS_HAS_ROT_P ( 1 << 9 )
#define FLAGS_HAS_ROT_R ( 1 << 10 )

#define FLAGS_HAS_TRANS_X ( 1 << 11 )
#define FLAGS_HAS_TRANS_Y ( 1 << 12 )
#define FLAGS_HAS_TRANS_Z ( 1 << 13 )
#define FLAGS_HAS_TRANS_ANY ( 7 << 11 )
#define FLAGS_HAS_TRANS_LARGE ( 1 << 14 )
#define FLAGS_QUAT0_NEG ( 1 << 15 )

using uint32 = uint32_t;

#define RwStreamRead(f, p, s) fread(p, s, 1, f)
#define debug(...) // printf(__VA_ARGS__)
#define RwMalloc malloc

struct rotation_t {
    float y; // Theta, in [0, π/2]
    float p; // phi, typically in [-π, π]
    float r; // psi, typically in [-π, π]

    uint16_t fixed_y() {
        return fixed(y);
    }

    uint16_t fixed_p() {
        return fixed(p);
    }

    uint16_t fixed_r() {
        return fixed(r);
    }

    static uint16_t fixed(float a) {
        // we use int16_t bellow instead since overflows are 2 * M_PI
        // if (a < 0)
        //     a += 2 * M_PI;
        // if (a > 2 * M_PI)
        //     a -= 2 * M_PI;
        // assert(a >= 0 && a < 2 * M_PI);
        return static_cast<int16_t>(a * 65536 / (2 * M_PI));
    }
};

struct quaternion_t {
    float x, y, z, w;

    quaternion_t inverted() {
        return {-x, -y, -z, w};
    }

    quaternion_t operator-() const {
        return {-x, -y, -z, -w};
    }

    rotation_t toSpherical() const {
        rotation_t rot;
        // Compute the magnitude of the first complex component (w, x)
        float A = sqrt(w * w + x * x);
        // Compute the magnitude of the second complex component (y, z)
        float B = sqrt(y * y + z * z);
        
        // Calculate theta from the ratio B/A.
        // A is non-negative by definition, and so is B.
        rot.y = atan2(B, A);
        
        // phi is the argument (angle) of the complex number A = w + i*x.
        rot.p = atan2(x, w);
        
        // psi is the argument of the complex number B = y + i*z.
        rot.r = atan2(z, y);
        return rot;
    }

    // Convert from spherical (yaw, pitch, roll) to quaternion
    static quaternion_t fromSpherical(const rotation_t& rot) {
        quaternion_t q;
        q.w = cos(rot.y) * cos(rot.p);
        q.x = cos(rot.y) * sin(rot.p);
        q.y = sin(rot.y) * cos(rot.r);
        q.z = sin(rot.y) * sin(rot.r);
        return q;
    }

    static quaternion_t fromSphericalFixed(uint16_t y, uint16_t p, uint16_t r) {
        quaternion_t q;
        q.w = cos((y / 65536.0f) * 2 * M_PI) * cos((p / 65536.0f) * 2 * M_PI);
        q.x = cos((y / 65536.0f) * 2 * M_PI) * sin((p / 65536.0f) * 2 * M_PI);
        q.y = sin((y / 65536.0f) * 2 * M_PI) * cos((r / 65536.0f) * 2 * M_PI);
        q.z = sin((y / 65536.0f) * 2 * M_PI) * sin((r / 65536.0f) * 2 * M_PI);
        return q;
    }
};
struct translation_t {
    float x, y, z;
};

void assert_float(float v, float exp, float espilon = 0.01) {
    assert(fabs(v - exp) < espilon);
}

void assert_trans(const translation_t& v, const translation_t& exp, float espilon = 2/128.f) {
    assert(fabs(v.x - exp.x) < espilon);
    assert(fabs(v.y - exp.y) < espilon);
    assert(fabs(v.z - exp.z) < espilon);
}

void assert_quat(const quaternion_t& v, const quaternion_t& exp, float espilon = 0.04) {
    assert(fabs(v.x - exp.x) < espilon);
    assert(fabs(v.y - exp.y) < espilon);
    assert(fabs(v.z - exp.z) < espilon);
    if (fabs(exp.x) < 0.0001f && fabs(exp.y) < 0.0001f && fabs(exp.z) < 0.0001f)
        ;
    else
        assert(fabs(v.w - exp.w) < espilon);
}

float dot(const quaternion_t &a, const quaternion_t &b) {
    return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
}

struct sequence_t {
    std::vector<quaternion_t> rotations;
    std::vector<translation_t> translations;
    std::vector<float> times;
    std::string name;
    int boneTag = -1;

    void removeQuaternionFlips() {
        quaternion_t q1 = rotations.front();
        for(size_t i = 1; i < rotations.size(); i++){
            auto q2 = rotations[i];
            float dot = q1.x*q2.x + q1.y*q2.y + q1.z*q2.z + q1.w*q2.w;
            if(dot < 0.0f)
                rotations[i] = -q2;
            q1 = rotations[i];
        }
    }
    
    // This function is assumed to be a member of your sequence_t type.
    void decompressDeltaCompressedData(const std::vector<uint8_t>& compData, sequence_t ref) {
        Reader reader(compData);
    
        // Read the flags written at the start.
        uint16_t flags = reader.read<uint16_t>();
    
        // Read initial and final timestamps.
        float initial_ts = reader.read<float>();
        float final_ts   = reader.read<float>();
    
        // Assume that the number of frames was set before calling this function.
        int nFrames = times.size();
        if (nFrames == 0)
            return;

        rotations.resize(nFrames);
        
        // Set first frame time.
        times[0] = initial_ts;
        float predicted_ts = initial_ts;
    
        // --- Translations ---
        float predicted_tx = 0, predicted_ty = 0, predicted_tz = 0;
        if (flags & FLAGS_KF_TRANS) {
            translation_t initTrans, finalTrans;
            translations.resize(nFrames);
            if (flags & FLAGS_HAS_TRANS_LARGE) {
                initTrans.x = reader.read<float>();
                initTrans.y = reader.read<float>();
                initTrans.z = reader.read<float>();
                predicted_tx = initTrans.x;
                predicted_ty = initTrans.y;
                predicted_tz = initTrans.z;
                translations[0] = initTrans;
                // Read final translation (may be used for verification or ignored)
                finalTrans.x = reader.read<float>();
                finalTrans.y = reader.read<float>();
                finalTrans.z = reader.read<float>();
            } else {
                initTrans.x = reader.read<int16_t>() / 128.f;
                initTrans.y = reader.read<int16_t>() / 128.f;
                initTrans.z = reader.read<int16_t>() / 128.f;
                predicted_tx = initTrans.x;
                predicted_ty = initTrans.y;
                predicted_tz = initTrans.z;
                translations[0] = initTrans;
                // Read final translation (for completeness)
                finalTrans.x = reader.read<int16_t>() / 128.f;
                finalTrans.y = reader.read<int16_t>() / 128.f;
                finalTrans.z = reader.read<int16_t>() / 128.f;
            }

            assert_trans(initTrans, ref.translations.front());
            
            assert_trans(finalTrans, ref.translations.back());
        }
    
        // --- Rotations ---
        // Read the absolute fixed‑point rotation values for the first frame.
        uint16_t fixed_y = reader.read<uint16_t>();
        uint16_t fixed_p = reader.read<uint16_t>();
        uint16_t fixed_r = reader.read<uint16_t>();
        rotations[0] = quaternion_t::fromSphericalFixed(fixed_y, fixed_p, fixed_r);
        uint16_t predicted_y = fixed_y;
        uint16_t predicted_p = fixed_p;
        uint16_t predicted_r = fixed_r;

        if (flags & FLAGS_QUAT0_NEG) {
            rotations[0] = -rotations[0];
        }

        assert_quat(rotations[0], ref.rotations.front());
    
        // --- Delta decoding for each subsequent frame ---
        for (int frameId = 1; frameId < nFrames; frameId++) {
            // --- Rotations ---
            // For rotation Y:
            if (flags & FLAGS_HAS_ROT_Y) {
                uint8_t byteVal = reader.read<uint8_t>();
                if (byteVal == 128) {
                    predicted_y = reader.read<uint16_t>();
                } else {
                    int8_t diff = static_cast<int8_t>(byteVal);
                    predicted_y += diff * 8;
                }
            }
            // For rotation P:
            if (flags & FLAGS_HAS_ROT_P) {
                uint8_t byteVal = reader.read<uint8_t>();
                if (byteVal == 128) {
                    predicted_p = reader.read<uint16_t>();
                } else {
                    int8_t diff = static_cast<int8_t>(byteVal);
                    predicted_p += diff * 8;
                }
            }
            // For rotation R:
            if (flags & FLAGS_HAS_ROT_R) {
                uint8_t byteVal = reader.read<uint8_t>();
                if (byteVal == 128) {
                    predicted_r = reader.read<uint16_t>();
                } else {
                    int8_t diff = static_cast<int8_t>(byteVal);
                    predicted_r += diff * 8;
                }
            }
            if (frameId < rotations.size()) {
                rotations[frameId] = quaternion_t::fromSphericalFixed(predicted_y, predicted_p, predicted_r);
            }
    
            // --- Translations ---
            // Translation X:
            if (flags & FLAGS_HAS_TRANS_X) {
                uint8_t byteVal = reader.read<uint8_t>();
                if (byteVal == 128) {
                    uint16_t diff = reader.read<uint16_t>();
                    if (diff != 32768) {
                        predicted_tx += static_cast<int16_t>(diff) / 128.f;
                    } else {
                        predicted_tx = reader.read<float>();
                    }
                } else {
                    int8_t diff = static_cast<int8_t>(byteVal);
                    predicted_tx += diff / 127.f;
                }
            }
            // Translation Y:
            if (flags & FLAGS_HAS_TRANS_Y) {
                uint8_t byteVal = reader.read<uint8_t>();
                if (byteVal == 128) {
                    uint16_t diff = reader.read<uint16_t>();
                    if (diff != 32768) {
                        predicted_ty += static_cast<int16_t>(diff) / 128.f;
                    } else {
                        predicted_ty = reader.read<float>();
                    }
                } else {
                    int8_t diff = static_cast<int8_t>(byteVal);
                    predicted_ty += diff / 127.f;
                }
            }
            // Translation Z:
            if (flags & FLAGS_HAS_TRANS_Z) {
                uint8_t byteVal = reader.read<uint8_t>();
                if (byteVal == 128) {
                    uint16_t diff = reader.read<uint16_t>();
                    if (diff != 32768) {
                        predicted_tz += static_cast<int16_t>(diff) / 128.f;
                    } else {
                        predicted_tz = reader.read<float>();
                    }
                } else {
                    int8_t diff = static_cast<int8_t>(byteVal);
                    predicted_tz += diff / 127.f;
                }
            }
            if (frameId < translations.size()) {
                translations[frameId] = { predicted_tx, predicted_ty, predicted_tz };
                assert_trans(translations[frameId], ref.translations[frameId]);
            }
    
            // --- Time Stamps ---
            {
                uint8_t byteValPacked = reader.read<uint8_t>();
                uint8_t byteVal = byteValPacked & 127;
                float diff;
                if (byteVal == 127) {
                    uint16_t fixed_diff = reader.read<uint16_t>();
                    diff = fixed_diff / 256.f;
                } else {
                    diff = byteVal / 256.f;
                }
                predicted_ts += diff;
                times[frameId] = predicted_ts;
                assert_float(times[frameId], ref.times[frameId]);

                if (byteValPacked & 128) {
                    rotations[frameId] = -rotations[frameId];
                }
            }

            // assert for quaternion now that flip has been done, if needed
            assert_quat(rotations[frameId], ref.rotations[frameId]);
        }
    
        // Optionally verify that the final reconstructed timestamp matches the stored final timestamp.
        if (fabs(predicted_ts - final_ts) > 0.001f) {
            fprintf(stderr, "Warning: final timestamp mismatch: %f vs %f\n", predicted_ts, final_ts);
        }
    }

    Writer getDeltaCompressedData() {
        Writer writer;
        uint16_t flags = 0;

        std::vector<float> diffs_ts(times.size()), ts(times.size());
        for (size_t i = 1; i < times.size(); i++) {
            float diff = times[i] - times[i-1];
            assert (diff >=0 && diff < 256);
            diffs_ts[i] = diff;
            ts[i] = times[i];
        }

        diffs_ts[0] = ts[0] = times[0];

        translation_t maxMag_trans = { 0, 0, 0};
        std::vector<translation_t> diffs_trans(translations.size()), trans(translations.size());
        if (translations.size()  >  0) {
            for (size_t i = translations.size() - 1; i != 0 ; i--) {
                translation_t prev = translations[i-1];
                translation_t curr = translations[i];
    
                translation_t diff;
                diff.x = curr.x - prev.x;
                diff.y = curr.y - prev.y;
                diff.z = curr.z - prev.z;
    
                diffs_trans[i] = diff;
                trans[i] = curr;
                
                maxMag_trans.x = std::max(maxMag_trans.x, std::abs(diff.x));
                maxMag_trans.y = std::max(maxMag_trans.y, std::abs(diff.y));
                maxMag_trans.z = std::max(maxMag_trans.z, std::abs(diff.z));
            }
    
            diffs_trans[0] = trans[0] = translations[0];
        }


        std::vector<rotation_t> diffs_rots(rotations.size()), rots(rotations.size());

        rotation_t maxMag_rots = { 0, 0, 0};
        for (size_t i = rotations.size() - 1; i != 0 ; i--) {
            rotation_t prev = rotations[i-1].toSpherical();
            rotation_t curr = rotations[i].toSpherical();

            rotation_t diff;
            diff.y = curr.y - prev.y;
            diff.p = curr.p - prev.p;
            diff.r = curr.r - prev.r;

            diffs_rots[i] = diff;
            rots[i] = curr;
            
            maxMag_rots.y = std::max(maxMag_rots.y, std::abs(diff.y));
            maxMag_rots.p = std::max(maxMag_rots.p, std::abs(diff.p));
            maxMag_rots.r = std::max(maxMag_rots.r, std::abs(diff.r));
        }
        diffs_rots[0] = rots[0] = rotations[0].toSpherical();

        // okay, now we have the diffs, the max magnitudes, and the original values
        // do the write out
        flags = FLAGS_KF_ROT; // always have rotations
        
        if (translations.size() > 0) {
            flags |= FLAGS_KF_TRANS;
        }

        {
            quaternion_t q1 = quaternion_t::fromSphericalFixed(rots.front().fixed_y(), rots.front().fixed_p(), rots.front().fixed_r());
            if (dot(rotations.front(), q1) < 0) {
                flags |= FLAGS_QUAT0_NEG;
            }
        }

        if (maxMag_rots.p > 0.0001) {
            flags |= FLAGS_HAS_ROT_P;
        }
        if (maxMag_rots.y > 0.0001) {
            flags |= FLAGS_HAS_ROT_Y;
        }
        if (maxMag_rots.r > 0.0001) {
            flags |= FLAGS_HAS_ROT_R;
        }

        if (maxMag_trans.x > 0.0001f) {
            flags |= FLAGS_HAS_TRANS_X;
        }
        if (maxMag_trans.y > 0.0001f) {
            flags |= FLAGS_HAS_TRANS_Y;
        }
        if (maxMag_trans.z > 0.0001f) {
            flags |= FLAGS_HAS_TRANS_Z;
        }

        if (trans.size() > 0) {
            if (fabs(trans.front().x) > 127 || fabs(trans.front().y) > 127 || fabs(trans.front().z) > 127) {
                flags |= FLAGS_HAS_TRANS_LARGE;
            }

            if (fabs(trans.back().x) > 127 || fabs(trans.back().y) > 127 || fabs(trans.back().z) > 127) {
                flags |= FLAGS_HAS_TRANS_LARGE;
            }
        }

        assert(ts[0] < 256);

        writer.write<uint16_t>(flags);

        writer.write<float>(ts.front());
        size_t end_time_offset = writer.size();
        writer.write<float>(0); // filler for end timestamp

        float predicted_ts = ts.front();

        float predicted_tx = 0, predicted_ty = 0, predicted_tz = 0;
        
        if (flags & FLAGS_KF_TRANS) {

            if (flags & FLAGS_HAS_TRANS_LARGE) {
                writer.write<float>(trans.front().x);
                writer.write<float>(trans.front().y);
                writer.write<float>(trans.front().z);

                predicted_tx = trans.front().x;
                predicted_ty = trans.front().y;
                predicted_tz = trans.front().z;

                writer.write<float>(trans.back().x);
                writer.write<float>(trans.back().y);
                writer.write<float>(trans.back().z);
            } else {
                writer.write<int16_t>(trans.front().x * 128);
                writer.write<int16_t>(trans.front().y * 128);
                writer.write<int16_t>(trans.front().z * 128);

                predicted_tx = int16_t(trans.front().x * 128) / 128.f;
                predicted_ty = int16_t(trans.front().y * 128) / 128.f;
                predicted_tz = int16_t(trans.front().z * 128) / 128.f;
    
                writer.write<int16_t>(trans.back().x * 128);
                writer.write<int16_t>(trans.back().y * 128);
                writer.write<int16_t>(trans.back().z * 128);
            }

            if (flags & FLAGS_HAS_TRANS_ANY) {
                translation_t t1 { predicted_tx, predicted_ty, predicted_tz };
                assert_trans(t1, trans[0]);
            }
        }

        writer.write<uint16_t>(rots.front().fixed_y());
        writer.write<uint16_t>(rots.front().fixed_p());
        writer.write<uint16_t>(rots.front().fixed_r());

        uint16_t predicted_y = rots.front().fixed_y();
        uint16_t predicted_p = rots.front().fixed_p();
        uint16_t predicted_r = rots.front().fixed_r();
        
        for (int frameId = 1; frameId < times.size(); frameId++) {
            if (flags & FLAGS_HAS_ROT_Y) {
                int16_t diff = rots[frameId].fixed_y() - predicted_y;
                if (abs(diff) > 127*8) {
                    writer.write<uint8_t>(128);
                    writer.write<uint16_t>(rots[frameId].fixed_y()); // special case: wraps around
                    predicted_y  = rots[frameId].fixed_y();
                } else {
                    writer.write<int8_t>(diff/8);
                    predicted_y += int8_t(diff/8) * 8;
                }
            }
            if (flags & FLAGS_HAS_ROT_P) {
                int16_t diff = rots[frameId].fixed_p() - predicted_p;
                if (abs(diff) > 127*8) {
                    writer.write<uint8_t>(128);
                    writer.write<uint16_t>(rots[frameId].fixed_p()); // special case: wraps around
                    predicted_p  = rots[frameId].fixed_p();
                } else {
                    writer.write<int8_t>(diff/8);
                    predicted_p += int8_t(diff/8) * 8;
                }
            }
            if (flags & FLAGS_HAS_ROT_R) {
                int16_t diff = rots[frameId].fixed_r() - predicted_r;
                if (abs(diff) > 127*8) {
                    writer.write<uint8_t>(128);
                    writer.write<uint16_t>(rots[frameId].fixed_r()); // special case: wraps around
                    predicted_r  = rots[frameId].fixed_r();
                } else {
                    writer.write<int8_t>(diff/8);
                    predicted_r += int8_t(diff/8) * 8;
                }
            }

            bool quat_flip = false;
            {
                quaternion_t q1 = quaternion_t::fromSphericalFixed(predicted_y, predicted_p, predicted_r);
                if (dot(rotations[frameId], q1) < 0) {
                    q1 = -q1;
                    quat_flip = true;
                }
                assert_quat(q1, rotations[frameId]);
            }

            if (flags & FLAGS_HAS_TRANS_X) {
                float diff = trans[frameId].x - predicted_tx;
                if (fabs(diff) > 1) {
                    if (fabs(diff) < 128) {
                        writer.write<uint8_t>(128);
                        writer.write<int16_t>(diff * 128);
                        predicted_tx += int16_t(diff * 128) / 128.f;
                    } else {
                        writer.write<uint8_t>(128);
                        writer.write<uint16_t>(32768);
                        writer.write<float>(trans[frameId].x);
                        predicted_tx = trans[frameId].x;
                    }
                } else {
                    int8_t fixed_diff = diff * 127;
                    writer.write<int8_t>(fixed_diff);
                    predicted_tx += fixed_diff / 127.f;
                }
            }
            if (flags & FLAGS_HAS_TRANS_Y) {
                float diff = trans[frameId].y - predicted_ty;
                if (fabs(diff) > 1) {
                    if (fabs(diff) < 128) {
                        writer.write<uint8_t>(128);
                        writer.write<int16_t>(diff * 128);
                        predicted_ty += int16_t(diff * 128) / 128.f;
                    } else {
                        writer.write<uint8_t>(128);
                        writer.write<uint16_t>(32768);
                        writer.write<float>(trans[frameId].y);
                        predicted_ty = trans[frameId].y;
                    }
                } else {
                    int8_t fixed_diff = diff * 127;
                    writer.write<int8_t>(fixed_diff);
                    predicted_ty += fixed_diff / 127.f;
                }
            }
            if (flags & FLAGS_HAS_TRANS_Z) {
                float diff = trans[frameId].z - predicted_tz;
                if (fabs(diff) > 1) {
                    if (fabs(diff) < 128) {
                        writer.write<uint8_t>(128);
                        writer.write<int16_t>(diff * 128);
                        predicted_tz += int16_t(diff * 128) / 128.f;
                    } else {
                        writer.write<uint8_t>(128);
                        writer.write<uint16_t>(32768);
                        writer.write<float>(trans[frameId].z);
                        predicted_tz = trans[frameId].z;
                    }
                } else {
                    int8_t fixed_diff = diff * 127;
                    writer.write<int8_t>(fixed_diff);
                    predicted_tz += fixed_diff / 127.f;
                }
            }

            if (flags & FLAGS_HAS_TRANS_ANY) {
                translation_t t1 { predicted_tx, predicted_ty, predicted_tz };
                assert_trans(t1, trans[frameId]);
            }
            
            {
                float diff = ts[frameId] - predicted_ts;
                uint16_t fixed_diff = diff * 256;
                if (fixed_diff >= 127) {
                    writer.write<uint8_t>(127 | quat_flip << 7);
                    writer.write<uint16_t>(fixed_diff);
                    predicted_ts += fixed_diff / 256.f;
                } else {
                    writer.write<uint8_t>(fixed_diff | quat_flip << 7);
                    predicted_ts += fixed_diff / 256.f;
                }

                assert_float(predicted_ts, ts[frameId]);
            }
        }

        memcpy(writer.data() + end_time_offset, &predicted_ts, sizeof(float));
        return writer;
    }
};

struct animation_t {
    std::vector<sequence_t> sequences;
    std::string name;
};

struct block_t {
    std::vector<animation_t> animations;
    std::string name;
};

#define ROUNDSIZE(x) if((x) & 3) (x) += 4 - ((x)&3)
struct IfpHeader {
    uint32_t ident;
    uint32 size;
};

block_t LoadAnimFile(FILE *stream)
{
    block_t block;

    size_t totalsize = 0;
    size_t totalframes = 0;

	IfpHeader anpk, info, name, dgan, cpan, anim;
	char buf[256];
	int j, k, l;
	float *fbuf = (float*)buf;

	// block name
	RwStreamRead(stream, &anpk, sizeof(IfpHeader));
	ROUNDSIZE(anpk.size);
	RwStreamRead(stream, &info, sizeof(IfpHeader));
	ROUNDSIZE(info.size);
	RwStreamRead(stream, buf, info.size);
	int numAnims = *(int*)buf;

	debug("Loading ANIMS %s\n", buf+4);
    block.name = buf+4;

	for(j = 0; j < numAnims; j++){

		// animation name
		RwStreamRead(stream, &name, sizeof(IfpHeader));
		ROUNDSIZE(name.size);
		RwStreamRead(stream, buf, name.size);
		
        debug("Animation: %s\n", buf);
        block.animations.push_back(animation_t());
        animation_t &animation = block.animations.back();
        animation.name = buf;

		// DG info has number of nodes/sequences
		RwStreamRead(stream, (char*)&dgan, sizeof(IfpHeader));
		ROUNDSIZE(dgan.size);
		RwStreamRead(stream, (char*)&info, sizeof(IfpHeader));
		ROUNDSIZE(info.size);
		RwStreamRead(stream, buf, info.size);
		int numSequences = *(int*)buf;

		for(k = 0; k < numSequences; k++){
            animation.sequences.push_back(sequence_t());
            sequence_t &seq = animation.sequences.back();

			// Each node has a name and key frames
			RwStreamRead(stream, &cpan, sizeof(IfpHeader));
			ROUNDSIZE(dgan.size);
			RwStreamRead(stream, &anim, sizeof(IfpHeader));
			ROUNDSIZE(anim.size);
			RwStreamRead(stream, buf, anim.size);
			int numFrames = *(int*)(buf+28);
		
            debug("Sequence: %s\n", buf);
            seq.name = buf;
			if(anim.size == 44)
				seq.boneTag = *(int*)(buf+40);
			if(numFrames == 0)
				continue;

			bool hasScale = false;
			bool hasTranslation = false;
			RwStreamRead(stream, &info, sizeof(info));
            size_t framesize = 0;
			if(strncmp((char*)&info.ident, "KRTS", 4) == 0){
				hasScale = true;
				// seq->SetNumFrames(numFrames, true, compressHier);
                framesize = 3 * 2 + 4 * 2 + 2;
			}else if(strncmp((char*)&info.ident, "KRT0", 4) == 0){
				hasTranslation = true;
				// seq->SetNumFrames(numFrames, true, compressHier);
                framesize = 3 * 2 + 4 * 2 + 2;
			}else if(strncmp((char*)&info.ident, "KR00", 4) == 0){
				// seq->SetNumFrames(numFrames, false, compressHier);
                framesize = 4 * 2 + 2;
			}

            size_t animsize = numFrames * framesize;
            totalsize += animsize;
            totalframes += numFrames;

			// float *frameTimes = (float*)RwMalloc(sizeof(float) * numFrames);
			debug("%d frames, %zu bytes\n", numFrames, animsize);
			for(l = 0; l < numFrames; l++){
				if(hasScale){
					RwStreamRead(stream, buf, 0x2C);
                    seq.rotations.push_back(quaternion_t{fbuf[0], fbuf[1], fbuf[2], fbuf[3]}.inverted());
                    seq.translations.push_back(translation_t{fbuf[4], fbuf[5], fbuf[6]});
                    seq.times.push_back(fbuf[10]);
					// CQuaternion rot(fbuf[0], fbuf[1], fbuf[2], fbuf[3]);
					// rot.Invert();
					// CVector trans(fbuf[4], fbuf[5], fbuf[6]);

					// seq->SetRotation(l, rot);
					// seq->SetTranslation(l, trans);
					// // scaling ignored
					// frameTimes[l] = fbuf[10];	// absolute time here
				}else if(hasTranslation){
					RwStreamRead(stream, buf, 0x20);
                    seq.rotations.push_back(quaternion_t{fbuf[0], fbuf[1], fbuf[2], fbuf[3]}.inverted());
                    seq.translations.push_back(translation_t{fbuf[4], fbuf[5], fbuf[6]});
                    seq.times.push_back(fbuf[7]);
					// CQuaternion rot(fbuf[0], fbuf[1], fbuf[2], fbuf[3]);
					// rot.Invert();
					// CVector trans(fbuf[4], fbuf[5], fbuf[6]);

					// seq->SetRotation(l, rot);
					// seq->SetTranslation(l, trans);
					// frameTimes[l] = fbuf[7];	// absolute time here
				}else{
					RwStreamRead(stream, buf, 0x14);
                    seq.rotations.push_back(quaternion_t{fbuf[0], fbuf[1], fbuf[2], fbuf[3]}.inverted());
                    seq.times.push_back(fbuf[4]);
					// CQuaternion rot(fbuf[0], fbuf[1], fbuf[2], fbuf[3]);
					// rot.Invert();

					// seq->SetRotation(l, rot);
					// frameTimes[l] = fbuf[4];	// absolute time here
				}
			}
            debug("Start time: %f, end time: %f\n", seq.times.front(), seq.times.back());

            seq.removeQuaternionFlips();
		}
	}
    return block;
}

void StoreAnimComprFile(FILE* stream, block_t block) {
    IfpHeader anpv = { 0, (uint32_t)block.name.size() + 1 };
    memcpy(&anpv.ident, "ANPV", 4);

    fwrite(&anpv, sizeof(IfpHeader), 1, stream);
    fwrite(block.name.c_str(), block.name.size() + 1, 1, stream);
    int numAnims = block.animations.size();
    fwrite(&numAnims, sizeof(numAnims), 1, stream);

    for (int animId = 0; animId < numAnims; animId++) {
        auto &anim = block.animations[animId];
        int animNameLength = anim.name.size() + 1;
        fwrite(&animNameLength, sizeof(animNameLength), 1, stream);
        fwrite(anim.name.c_str(), animNameLength, 1, stream);
        int numSeqs = anim.sequences.size();
        fwrite(&numSeqs, sizeof(numSeqs), 1, stream);

        for (int seqId = 0; seqId < numSeqs; seqId++) {
            auto &seq = anim.sequences[seqId];
            int seqNameLength = seq.name.size() + 1;
            fwrite(&seqNameLength, sizeof(seqNameLength), 1, stream);
            fwrite(seq.name.c_str(), seqNameLength, 1, stream);
            int numFrames = seq.times.size();
            fwrite(&numFrames, sizeof(numFrames), 1, stream);
            int boneTag = seq.boneTag;
            fwrite(&boneTag, sizeof(boneTag), 1, stream);

            if (numFrames == 0)
                continue;
            
            auto data = seq.getDeltaCompressedData();

            uint32_t dataSize = data.size();
            fwrite(&dataSize, sizeof(dataSize), 1, stream);
            fwrite(data.data(), dataSize, 1, stream);
        }
    }
}

block_t LoadAnimComprFile(FILE* stream, block_t ref) {
    block_t block;

    // Read and check header.
    IfpHeader header;
    if (fread(&header, sizeof(IfpHeader), 1, stream) != 1) {
        fprintf(stderr, "Error reading IFP header.\n");
        exit(EXIT_FAILURE);
    }
    if (memcmp(&header.ident, "ANPV", 4) != 0) {
        fprintf(stderr, "Invalid file header. Expected 'ANPV'.\n");
        exit(EXIT_FAILURE);
    }

    // Read block name. (header.size equals block.name.size() + 1)
    std::vector<char> blockName(header.size);
    if (fread(blockName.data(), header.size, 1, stream) != 1) {
        fprintf(stderr, "Error reading block name.\n");
        exit(EXIT_FAILURE);
    }
    block.name = std::string(blockName.data());

    // Read number of animations.
    int numAnims = 0;
    if (fread(&numAnims, sizeof(numAnims), 1, stream) != 1) {
        fprintf(stderr, "Error reading number of animations.\n");
        exit(EXIT_FAILURE);
    }
    block.animations.resize(numAnims);

    // For each animation...
    for (int animId = 0; animId < numAnims; animId++) {
        animation_t anim;

        // Read animation name.
        int animNameLength = 0;
        if (fread(&animNameLength, sizeof(animNameLength), 1, stream) != 1) {
            fprintf(stderr, "Error reading animation name length.\n");
            exit(EXIT_FAILURE);
        }
        std::vector<char> animName(animNameLength);
        if (fread(animName.data(), animNameLength, 1, stream) != 1) {
            fprintf(stderr, "Error reading animation name.\n");
            exit(EXIT_FAILURE);
        }
        anim.name = std::string(animName.data());

        // Read number of sequences.
        int numSeqs = 0;
        if (fread(&numSeqs, sizeof(numSeqs), 1, stream) != 1) {
            fprintf(stderr, "Error reading number of sequences.\n");
            exit(EXIT_FAILURE);
        }
        anim.sequences.resize(numSeqs);

        // For each sequence...
        for (int seqId = 0; seqId < numSeqs; seqId++) {
            sequence_t seq;

            // Read sequence name.
            int seqNameLength = 0;
            if (fread(&seqNameLength, sizeof(seqNameLength), 1, stream) != 1) {
                fprintf(stderr, "Error reading sequence name length.\n");
                exit(EXIT_FAILURE);
            }
            std::vector<char> seqName(seqNameLength);
            if (fread(seqName.data(), seqNameLength, 1, stream) != 1) {
                fprintf(stderr, "Error reading sequence name.\n");
                exit(EXIT_FAILURE);
            }
            seq.name = std::string(seqName.data());

            // Read number of frames.
            int numFrames = 0;
            if (fread(&numFrames, sizeof(numFrames), 1, stream) != 1) {
                fprintf(stderr, "Error reading number of frames.\n");
                exit(EXIT_FAILURE);
            }
            // Prepare to store times (and, by convention, other per-frame data).
            seq.times.resize(numFrames);

            // Read bone tag.
            int boneTag = 0;
            if (fread(&boneTag, sizeof(boneTag), 1, stream) != 1) {
                fprintf(stderr, "Error reading bone tag.\n");
                exit(EXIT_FAILURE);
            }
            seq.boneTag = boneTag;

            // If there are keyframes, read the delta compressed data.
            if (numFrames > 0) {
                uint32_t dataSize = 0;
                if (fread(&dataSize, sizeof(dataSize), 1, stream) != 1) {
                    fprintf(stderr, "Error reading delta compressed data size.\n");
                    exit(EXIT_FAILURE);
                }
                std::vector<uint8_t> compData(dataSize);
                if (fread(compData.data(), dataSize, 1, stream) != 1) {
                    fprintf(stderr, "Error reading delta compressed data.\n");
                    exit(EXIT_FAILURE);
                }

                // Decompress the data.
                seq.decompressDeltaCompressedData(compData, ref.animations[animId].sequences[seqId]);
            }

            anim.sequences[seqId] = seq;
        }

        block.animations[animId] = anim;
    }

    return block;
}


int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.ifp> <output.ifc>\n", argv[0]);
        return 1;
    }

    FILE *stream = fopen(argv[1], "rb");
    if (!stream) {
        fprintf(stderr, "Error: Unable to open input file %s\n", argv[1]);
        return 1;
    }

    auto block = LoadAnimFile(stream);

    fclose(stream);


    stream = fopen(argv[2], "wb");
    if (!stream) {
        fprintf(stderr, "Error: Unable to open output file %s\n", argv[2]);
        return 2;
    }
    StoreAnimComprFile(stream, block);
    fclose(stream);
    stream = fopen(argv[2], "rb");
    if (!stream) {
        fprintf(stderr, "Error: Unable to open output file %s\n", argv[2]);
        return 2;
    }
    auto decompressedBlock = LoadAnimComprFile(stream, block);
    fclose(stream);

    return 0;
}