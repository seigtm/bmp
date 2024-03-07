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

// Namespace with bmp related constants and file paths.
namespace setm::bmp {

#pragma pack(push, 1)
struct bitmap_file_header {
    std::uint16_t bf_type;
    std::uint32_t bf_size;
    std::uint16_t bf_reserved1;
    std::uint16_t bf_reserved2;
    std::uint32_t bf_off_bits;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct bitmap_info_header {
    std::uint32_t bi_size;
    std::int32_t bi_width;
    std::int32_t bi_height;
    std::uint16_t bi_planes;
    std::uint16_t bi_bit_count;
    std::uint32_t bi_compression;
    std::uint32_t bi_size_image;
    std::int32_t bi_x_pels_per_meter;
    std::int32_t bi_y_pels_per_meter;
    std::uint32_t bi_clr_used;
    std::uint32_t bi_clr_important;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct rgb_quad {
    std::uint8_t blue;
    std::uint8_t green;
    std::uint8_t red;
    std::uint8_t reserved;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct rgb_triple {
    std::uint8_t blue;
    std::uint8_t green;
    std::uint8_t red;
};
#pragma pack(pop)


static const auto assets_directory{ std::filesystem::current_path().append("assets") };
static const auto input_bmp_file_path{ assets_directory / "input.bmp" };

// The header field used to identify the BMP and DIB file is 0x42 0x4D in hexadecimal, same as BM in ASCII:
//   (https://en.wikipedia.org/wiki/BMP_file_format#Bitmap_file_header).
// Bytes are in reverse order because of little endian architecture:
//   (https://en.wikipedia.org/wiki/Endianness).
static const auto bmp_signature{ 0x4D42 };

};  // namespace setm::bmp

int main() {
    using namespace setm::bmp;

    std::ifstream input_bmp_file{ input_bmp_file_path, std::ios::binary };
    if(!input_bmp_file) {
        std::cerr << "Failed to open input file\n";
        return EXIT_FAILURE;
    }

    // Read bitmap headers.
    bitmap_file_header bmp_file_header;
    input_bmp_file.read(reinterpret_cast<char *>(&bmp_file_header), sizeof(bitmap_file_header));
    bitmap_info_header bmp_info_header;
    input_bmp_file.read(reinterpret_cast<char *>(&bmp_info_header), sizeof(bitmap_info_header));

    // Check if the file is a BMP file.
    if(bmp_file_header.bf_type != bmp_signature) {
        std::cerr << "File is not a BMP file\n";
        return EXIT_FAILURE;
    }

    // Output the dimensions and number of bits per pixel (8 - 24 bits).
    //   BITMAPINFOHEADER::biWidth (4B) - Width of the image in pixels.
    //   BITMAPINFOHEADER::biHeight (4B) - Height of the image in pixels.
    //   BITMAPINFOHEADER::biBitCount (2B) - Number of bits per pixel (1, 4, 8, 16, 24, 32).
    std::cout << "Width: " << bmp_info_header.bi_width << " Height: " << bmp_info_header.bi_height
              << "\nNumber of bits per pixel: " << bmp_info_header.bi_bit_count << '\n';
}
