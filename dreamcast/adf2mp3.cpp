#include <iostream>
#include <fstream>

int main(int argc, char* argv[]) {
    // Check for correct number of arguments.
    if (argc != 3) {
        std::cerr << "Usage: adf2mp3 <inputfile> <outputfile>" << std::endl;
        return 1;
    }

    // Get file names from command line arguments.
    const char* inputFileName = argv[1];
    const char* outputFileName = argv[2];

    // Open the input file in binary mode.
    std::ifstream inputFile(inputFileName, std::ios::binary);
    if (!inputFile) {
        std::cerr << "Error: Unable to open input file " << inputFileName << std::endl;
        return 1;
    }

    // Open the output file in binary mode.
    std::ofstream outputFile(outputFileName, std::ios::binary);
    if (!outputFile) {
        std::cerr << "Error: Unable to open output file " << outputFileName << std::endl;
        return 1;
    }

    // Buffer to hold file data.
    const size_t bufferSize = 4096;
    char buffer[bufferSize];

    // Process the file in chunks.
    while (inputFile.read(buffer, bufferSize) || inputFile.gcount() > 0) {
        // Get the number of bytes actually read.
        std::streamsize bytesRead = inputFile.gcount();
        // XOR each byte with 0x22.
        for (std::streamsize i = 0; i < bytesRead; ++i) {
            buffer[i] ^= 0x22;
        }
        // Write the modified buffer to the output file.
        outputFile.write(buffer, bytesRead);
    }

    // Close files.
    inputFile.close();
    outputFile.close();

    return 0;
}
