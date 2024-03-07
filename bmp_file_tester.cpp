/** @file bmp_file_tester.cpp
 *  @brief BMP file tester.
 *  @details This program tests an input BMP file and outputs its dimensions and number of bits per pixel (8 - 24 bits).
 *  @author Baranov Konstantin (seigtm) <gh@seig.ru>
 *  @version 1.0
 *  @date 2024-02-18
 */

#include <filesystem>
#include <fstream>
#include <iostream>

#include "Windows.h"

// Namespace with bmp related constants and file paths.
namespace setm::bmp {

static const auto assets_directory{ std::filesystem::current_path().append("assets") };
static const auto input_bmp_file_path{ assets_directory / "input.bmp" };

// The header field used to identify the BMP and DIB file is 0x42 0x4D in hexadecimal, same as BM in ASCII:
//   (https://en.wikipedia.org/wiki/BMP_file_format#Bitmap_file_header).
// Bytes are in reverse order because of little endian architecture:
//   (https://en.wikipedia.org/wiki/Endianness).
static const auto bmp_signature{ 0x4D42 };

};  // namespace setm::bmp

int main() {
    std::ifstream input_bmp_file{ setm::bmp::input_bmp_file_path, std::ios::binary };
    if(!input_bmp_file) {
        std::cerr << "Failed to open input file\n";
        return EXIT_FAILURE;
    }

    // Read bitmap headers.
    BITMAPFILEHEADER bmp_file_header;
    input_bmp_file.read(reinterpret_cast<char *>(&bmp_file_header), sizeof(BITMAPFILEHEADER));
    BITMAPINFOHEADER bmp_info_header;
    input_bmp_file.read(reinterpret_cast<char *>(&bmp_info_header), sizeof(BITMAPINFOHEADER));

    // Check if the file is a BMP file.
    if(bmp_file_header.bfType != setm::bmp::bmp_signature) {
        std::cerr << "File is not a BMP file\n";
        return EXIT_FAILURE;
    }

    // Output the dimensions and number of bits per pixel (8 - 24 bits).
    //   BITMAPINFOHEADER::biWidth (4B) - Width of the image in pixels.
    //   BITMAPINFOHEADER::biHeight (4B) - Height of the image in pixels.
    //   BITMAPINFOHEADER::biBitCount (2B) - Number of bits per pixel (1, 4, 8, 16, 24, 32).
    std::cout << "Width: " << bmp_info_header.biWidth << " Height: " << bmp_info_header.biHeight
              << "\nNumber of bits per pixel: " << bmp_info_header.biBitCount << '\n';
}
