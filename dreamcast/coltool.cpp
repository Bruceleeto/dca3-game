#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cassert>
#include <cstring>
#include <cmath>

// Our collision file header. In the original code the header is read from the file.
// Here we assume it contains an identifier and a size.
struct ColHeader {
    int32_t ident; // should be 'LLOC'
    int32_t size;  // size of the following collision data chunk
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: repack_collision <input_file> <output_file>\n";
        return 1;
    }
    const char* inputFile = argv[1];
    const char* outputFile = argv[2];

    std::ifstream infile(inputFile, std::ios::binary);
    if (!infile) {
        std::cerr << "Error: Cannot open input file: " << inputFile << "\n";
        return 1;
    }
    std::ofstream outfile(outputFile, std::ios::binary);
    if (!outfile) {
        std::cerr << "Error: Cannot open output file: " << outputFile << "\n";
        return 1;
    }

    // Process each collision model in the file.
    // Each chunk starts with a ColHeader followed by header.size bytes.
    while (infile.peek() != EOF) {
        ColHeader header;
        infile.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!infile) break;
        
        // Check that the header ident is correct (should equal 'LLOC')
        if (header.ident != 'LLOC') {
            auto curpos = infile.tellg();
            infile.seekg(0, infile.end);
            auto remaining = infile.tellg() - curpos;

            if (remaining < 2048) {
                return 0;
            } else {
                std::cerr << "Error: Invalid header identifier encountered.\n";
                return 1;
            }
        }

        header.ident = 'CLOC';

        // Read the collision chunk into a work buffer.
        std::vector<char> work_buff(header.size);
        infile.read(work_buff.data(), header.size);
        if (!infile) break;

        // The first 24 bytes of work_buff are the model name.
        // The rest of the buffer is the collision model data.
        // We now “repackage” the collision model data into a new vector.
        std::vector<char> out_buff;
        // Copy model name (first 24 bytes)
        out_buff.insert(out_buff.end(), work_buff.begin(), work_buff.begin() + 24);

        // Set an offset into work_buff where the collision model data begins.
        size_t offset = 24;

        // === Process Collision Model Data ===
        // Copy first 44 bytes (bounding sphere, bounding box, and extra bytes)
        out_buff.insert(out_buff.end(), work_buff.begin() + offset, work_buff.begin() + offset + 44);
        // We need to extract numSpheres which is stored at offset 40 (within collision data)
        int16_t numSpheres = *reinterpret_cast<int16_t*>(&work_buff[24 + 40]);
        offset += 44;

        assert(work_buff.size() > offset);
        // Copy each sphere data block (20 bytes per sphere)
        for (int i = 0; i < numSpheres; i++) {
            out_buff.insert(out_buff.end(), work_buff.begin() + offset, work_buff.begin() + offset + 20);
            offset += 20;
        }

        // Process lines: read 4 bytes (includes numLines)
        int16_t numLines = *reinterpret_cast<int16_t*>(&work_buff[offset]);
        out_buff.insert(out_buff.end(), work_buff.begin() + offset, work_buff.begin() + offset + 4);
        offset += 4;
        // For each line, copy 24 bytes
        for (int i = 0; i < numLines; i++) {
            out_buff.insert(out_buff.end(), work_buff.begin() + offset, work_buff.begin() + offset + 24);
            offset += 24;
        }

        // Process boxes: next 4 bytes hold numBoxes
        int16_t numBoxes = *reinterpret_cast<int16_t*>(&work_buff[offset]);
        out_buff.insert(out_buff.end(), work_buff.begin() + offset, work_buff.begin() + offset + 4);
        offset += 4;
        // For each box, copy 28 bytes
        for (int i = 0; i < numBoxes; i++) {
            assert(work_buff.size() > offset);
            out_buff.insert(out_buff.end(), work_buff.begin() + offset, work_buff.begin() + offset + 28);
            offset += 28;
        }

        // Process vertices: next 4 bytes (numVertices)
        int16_t numVertices = *reinterpret_cast<int16_t*>(&work_buff[offset]);
        out_buff.insert(out_buff.end(), work_buff.begin() + offset, work_buff.begin() + offset + 4);
        offset += 4;
        assert(work_buff.size() > offset);

        // For each vertex, convert to fixed point (6 bytes for every 12 bytes in)
        for (int i = 0; i < numVertices; i++) {
            for (int v = 0; v < 3; v++) {
                assert(work_buff.size() > offset);
                float d = *(float*)(work_buff.data() + offset);
                offset += 4;
                auto fix = llround(d * 128);
                assert(fix >= -32768 && fix <32767);
                int16_t fix16 = int16_t(fix);
                out_buff.insert(out_buff.end(), (uint8_t*)(&fix16), (uint8_t*)(&fix16 + 1));
            }
        }

        // Process triangles: next 4 bytes (numTriangles)
        int16_t numTriangles = *reinterpret_cast<int16_t*>(&work_buff[offset]);
        out_buff.insert(out_buff.end(), work_buff.begin() + offset, work_buff.begin() + offset + 4);
        offset += 4;
        // For each triangle, copy 16 bytes
        for (int i = 0; i < numTriangles; i++) {
            for (int v = 0; v < 3; v++) {
                int32_t d = *(int32_t*)(work_buff.data() + offset);
                offset += 4;
                assert(d >= -32768 && d <32767);
                assert(d < numVertices);
                int16_t d16 = int16_t(d);
                out_buff.insert(out_buff.end(), (uint8_t*)(&d16), (uint8_t*)(&d16 + 1));
            }

            out_buff.push_back(work_buff[offset]);
            out_buff.push_back(0); // padding
            offset += 4;
        }
        // === End repackaging of collision model data ===

        // align to 4 bytes
        while (out_buff.size() & 3) {
            out_buff.push_back(0);
        }

        // Update header.size to match our repackaged chunk size.
        header.size = static_cast<int32_t>(out_buff.size());

        // Write the header and the repackaged collision model data to the output file.
        outfile.write(reinterpret_cast<char*>(&header), sizeof(header));
        outfile.write(out_buff.data(), out_buff.size());
    }

    return 0;
}
