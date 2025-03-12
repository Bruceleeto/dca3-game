#include <cstdint>
#include <iostream>
#include <cassert>
#include <string>
#include <cstring>

#include <dirent.h>
#include <unistd.h>

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

#ifdef _WIN32
#define fcaseopen fopen
#else
// Case-insensitivity on linux (from https://github.com/OneSadCookie/fcaseopen)
// Returned string should freed manually (if exists)
char* casepath(char const* path, bool checkPathFirst = true)
{
    //TODO: Implement this
    bool access_ok = false; //access(path, F_OK) != -1
    // printf("TODO: FIXME %s\n", __func__);

    if (checkPathFirst && access_ok ) {
        // File path is correct
        return nullptr;
    }

    size_t l = strlen(path);
    if (l > 2 && path[0] == '.' && (path[1] == '/' || path[1] == '\\')) {
        // remove ./ from the start of the path
        path += 2;
    }
    char* p = (char*)alloca(l + 1);
    char* out = (char*)malloc(l + 3); // for extra ./
    strcpy(p, path);

    size_t rl = 0;

    DIR* d;
    char* c;

    #if defined(__SWITCH__) || defined(PSP2)
    if( (c = strstr(p, ":/")) != NULL) // scheme used by some environments, eg. switch, vita
    {
        size_t deviceNameOffset = c - p + 3;
        char* deviceNamePath = (char*)alloca(deviceNameOffset + 1);
        strlcpy(deviceNamePath, p, deviceNameOffset);
        deviceNamePath[deviceNameOffset] = 0;
        d = opendir(deviceNamePath);
        p = c + 1;
    }
    else
    #endif
    if (p[0] == '/' || p[0] == '\\')
    {
        d = opendir("/");
    }
    else
    {
        d = opendir(".");
        out[0] = '.';
        out[1] = 0;
        rl = 1;
    }

    bool cantProceed = false; // just convert slashes in what's left in string, don't correct case of letters(because we can't)
    bool mayBeTrailingSlash = false;

    while (c = strsep(&p, "/\\"))
    {
        // May be trailing slash(allow), slash at the start(avoid), or multiple slashes(avoid)
        if (*c == '\0')
        {
            mayBeTrailingSlash = true;
            continue;
        } else {
            mayBeTrailingSlash = false;
        }

        out[rl] = '/';
        rl += 1;
        out[rl] = 0;

        if (cantProceed)
        {
            strcpy(out + rl, c);
            rl += strlen(c);
            continue;
        }

        struct dirent* e;
        while (e = readdir(d))
        {
            if (strcasecmp(c, e->d_name) == 0)
            {
                strcpy(out + rl, e->d_name);
                int reportedLen = (int)strlen(e->d_name);
                rl += reportedLen;
                assert(reportedLen == strlen(c) && "casepath: This is not good at all");

                closedir(d);
                d = opendir(out);

                // Either it wasn't a folder, or permission error, I/O error etc.
                if (!d) {
                    cantProceed = true;
                }

                break;
            }
        }

        if (!e)
        {
            printf("casepath couldn't find dir/file \"%s\", full path was %s\n", c, path);
            // No match, add original name and continue converting further slashes.
            strcpy(out + rl, c);
            rl += strlen(c);
            cantProceed = true;
        }
    }

    if (d) closedir(d);
    if (mayBeTrailingSlash) {
        out[rl] = '/';  rl += 1;
        out[rl] = '\0';
    }

    if (rl > l + 2) {
        printf("\n\ncasepath: Corrected path length is longer then original+2:\n\tOriginal: %s (%zu chars)\n\tCorrected: %s (%zu chars)\n\n", path, l, out, rl);
    }
    return out;
}

FILE* fcaseopen(char const* filename, char const* mode)
{
    FILE* result;
    char* real = casepath(filename);
    if (!real)
        result = fopen(filename, mode);
    else {
        result = fopen(real, mode);
        free(real);
    }
    return result;
}
#endif

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

    FILE* fout=fcaseopen(argv[3], "wb");
    if (!fout) {
        std::cerr << "Failed to open " << argv[3] << " for writing" << std::endl;
        return 1;
    }

    for (int i = 0; i < table_len; i++) {
        std::string filename = std::string(argv[2]) + "/" + table[i];
        
        FILE* f = fcaseopen(filename.c_str(), "rb");
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