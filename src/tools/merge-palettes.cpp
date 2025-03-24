#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_root_folder> <output_file>" << std::endl;
        return 1;
    }

    std::string inputFolder = argv[1];
    std::string outputFileName = argv[2];

    // Open the output file for writing in binary mode.
    std::ofstream outFile(outputFileName, std::ios::binary);
    if (!outFile) {
        std::cerr << "Error: Cannot open output file " << outputFileName << std::endl;
        return 1;
    }

    // Process each file from 0.pal to 63.pal.
    for (int i = 0; i < 64; ++i) {
        std::string filePath = inputFolder + "/" + std::to_string(i) + ".pal";
        std::ifstream inFile(filePath, std::ios::binary);
        if (!inFile) {
            std::cerr << "Error: Cannot open input file " << filePath << std::endl;
            return 1;
        }

        // Read header (first 4 bytes). They should equal "DPAL".
        char header[4];
        inFile.read(header, 4);
        if (inFile.gcount() != 4 || std::string(header, 4) != "DPAL") {
            std::cerr << "Error: File " << filePath << " has an invalid header." << std::endl;
            return 1;
        }

        // Read the next 4 bytes (number of palette entries).
        uint32_t numEntries = 0;
        inFile.read(reinterpret_cast<char*>(&numEntries), sizeof(numEntries));
        if (inFile.gcount() != sizeof(numEntries)) {
            std::cerr << "Error: Could not read palette entry count from " << filePath << std::endl;
            return 1;
        }
        if (numEntries != 16) {
            std::cerr << "Error: File " << filePath << " has " << numEntries 
                      << " entries, expected 16." << std::endl;
            return 1;
        }

        // Read the remaining bytes (the palette values).
        std::vector<char> paletteData((std::istreambuf_iterator<char>(inFile)),
                                      std::istreambuf_iterator<char>());
        inFile.close();

        // Write palette data to the output file.
        outFile.write(paletteData.data(), paletteData.size());
        if (!outFile) {
            std::cerr << "Error: Writing to output file failed." << std::endl;
            return 1;
        }
    }

    outFile.close();
    std::cout << "Successfully combined 64 .pal files into " << outputFileName << std::endl;
    return 0;
}
