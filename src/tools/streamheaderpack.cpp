#include <cstdint>
#include <iostream>
#include <cassert>
#include <string>

struct WavHeader {
    // RIFF Header
    char riff[4];        // RIFF Header Magic header
    uint32_t chunkSize;  // RIFF Chunk Size
    char wave[4];        // WAVE Header
    // "fmt" sub-chunk
    char fmt[4];         // FMT header
    uint32_t subchunk1Size; // Size of the fmt chunk
    uint16_t audioFormat;   // Audio format 1=PCM, other values indicate compression
    uint16_t numOfChan;     // Number of channels 1=Mono, 2=Stereo
    uint32_t samplesPerSec; // Sampling Frequency in Hz
    uint32_t bytesPerSec;   // bytes per second
    uint16_t blockAlign;    // 2=16-bit mono, 4=16-bit stereo
    uint16_t bitsPerSample; // Number of bits per sample
    // "data" sub-chunk
    char data[4];        // "data" string
    uint32_t dataSize;   // Size of the data section
};

#define DCStreamedNameTable DCStreamedNameTable_liberty
#include "../liberty/audio/sampman_dc_streams.h"
#undef DCStreamedNameTable
#define DCStreamedNameTable DCStreamedNameTable_miami
#include "../miami/audio/sampman_dc_streams.h"
#undef DCStreamedNameTable
int main(int argc, const char** argv) {

    size_t table_len = 0;
    const char (* table)[25] = nullptr;

    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << "liberty|miami <input_dir> <output_file>" << std::endl;
        return 1;
    }

    if (strcmp(argv[1], "miami") == 0) {
        table = DCStreamedNameTable_miami;
        table_len = sizeof(DCStreamedNameTable_miami)/sizeof(DCStreamedNameTable_miami[0]);
    } else if (strcmp(argv[1], "liberty") == 0) {
        table = DCStreamedNameTable_liberty;
        table_len = sizeof(DCStreamedNameTable_liberty)/sizeof(DCStreamedNameTable_liberty[0]);
    } else {
        std::cerr << "Invalid game name" << std::endl;
        return 1;
    }

    FILE* fout=fopen(argv[3], "wb");
    if (!fout) {
        std::cerr << "Failed to open " << argv[3] << " for writing" << std::endl;
        return 1;
    }

    for (int i = 0; i < table_len; i++) {
        std::string filename = std::string(argv[2]) + "/" + table[i];
        
        FILE* f = fopen(filename.c_str(), "rb");
        if (!f) {
            std::cerr << "Failed to open " << filename << std::endl;
            fclose(fout);
            return 1;
        }
        WavHeader hdr;
        auto rv = fread(&hdr, sizeof(hdr), 1, f);
        if (rv != 1) {
            std::cerr << "Failed to read header from " << filename << std::endl;
            fclose(f);
            fclose(fout);
            return 1;
        }

        uint64_t sl64 = (uint64_t)hdr.dataSize * 2000 / hdr.numOfChan / hdr.samplesPerSec;

		assert(sl64 <= INT32_MAX);

        int32_t sl32 = (int32_t)sl64;

        rv = fwrite(&sl32, sizeof(sl32), 1, fout);
        if (rv != 1) {
            std::cerr << "Failed to write to " << argv[2] << std::endl;
            fclose(f);
            fclose(fout);
            return 1;
        }
        fclose(f);
    }
    fclose(fout);

    return 0;
}