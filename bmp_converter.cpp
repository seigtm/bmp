/** @file bmp_converter.cpp
 *  @brief BMP format converter.
 *  @details This program converts a 24-bit depth BMP image to 4-bit depth BMP image.
 *  @author Baranov Konstantin (seigtm) <gh@seig.ru>
 *  @version 1.0
 *  @date 2024-02-18
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <utility>

#include "Windows.h"

namespace fs = std::filesystem;

namespace setm::bmp {

namespace constants {

static const auto assets_directory{ fs::current_path().append("assets") };
static const auto target_bitcount{ 4 };
static const auto input_bmp_file_path{ assets_directory / "input.bmp" };
static const auto output_bmp_file_path{ assets_directory / "output_4bit.bmp" };
// The header field used to identify the BMP and DIB file is 0x42 0x4D in hexadecimal, same as BM in ASCII:
//   (https://en.wikipedia.org/wiki/BMP_file_format#Bitmap_file_header).
// Bytes are in reverse order because of little endian architecture:
//   (https://en.wikipedia.org/wiki/Endianness).
static constexpr auto BMP_SIGNATURE{ 0x4D42 };

};  // namespace constants

namespace utils {

constexpr double color_distance(const RGBTRIPLE &color1, const RGBQUAD &color2) noexcept {
    return std::sqrt(std::pow((color2.rgbBlue - color1.rgbtBlue), 2) +
                     std::pow((color2.rgbGreen - color1.rgbtGreen), 2) +
                     std::pow((color2.rgbRed - color1.rgbtRed), 2));
}

template<size_t N>
constexpr std::byte find_closest_color(const RGBTRIPLE &color, const std::array<RGBQUAD, N> &palette) noexcept {
    const auto distance_to_color{ [&color](const auto &lhs, const auto &rhs) {
        return color_distance(color, lhs) < color_distance(color, rhs);
    } };
    return static_cast<std::byte>(
        std::distance(std::begin(palette), std::ranges::min_element(palette, distance_to_color)));
}

}  // namespace utils

// Convert a 24-bit BMP image to a 4-bit.
void convert_bmp_24_to_4_depth(const fs::path &input_file_path,
                               const fs::path &output_file_path) {
    // Open input BMP file.
    std::ifstream input_file{ input_file_path, std::ios::binary };
    if(!input_file) {
        std::cerr << "Failed to open input file" << input_file_path << '\n';
        return;
    }

    // Read BMP headers.
    BITMAPFILEHEADER bmp_file_header;
    input_file.read(reinterpret_cast<char *>(&bmp_file_header), sizeof(BITMAPFILEHEADER));
    BITMAPINFOHEADER bmp_info_header;
    input_file.read(reinterpret_cast<char *>(&bmp_info_header), sizeof(BITMAPINFOHEADER));

    // Check if the file is a BMP file.
    if(bmp_file_header.bfType != constants::BMP_SIGNATURE) {
        std::cerr << "File " << input_file_path << " is not a BMP file\n";
        return;
    }

    // Check if the number of bits per pixel is 24.
    if(bmp_info_header.biBitCount != 24) {
        std::cerr << "File " << input_file_path << " has not 24 bits per pixel\n";
        return;
    }

    // Open output BMP file.
    std::ofstream output_file{ output_file_path, std::ios::binary };
    if(!output_file) {
        std::cerr << "Failed to open output file " << output_file_path << '\n';
        return;
    }

    // The Super Cassette Vision, equipped with an EPOCH TV-1 video processor, uses a 16-color palette.
    //   (https://en.wikipedia.org/wiki/List_of_video_game_console_palettes).
    constexpr static std::array palette{
        // Using BGRA color order.
        RGBQUAD{ 0x00, 0x00, 0x00, 0x00 },  // #000000 (Black).
        RGBQUAD{ 0x00, 0x00, 0xFF, 0x00 },  // #ff0000 (Red).
        RGBQUAD{ 0x00, 0xA1, 0xFF, 0x00 },  // #ffa100 (Orange).
        RGBQUAD{ 0x9F, 0xA0, 0xFF, 0x00 },  // #ffa09f (Light Red).
        RGBQUAD{ 0x00, 0xFF, 0xFF, 0x00 },  // #ffff00 (Yellow).
        RGBQUAD{ 0x00, 0xA0, 0xA3, 0x00 },  // #a3a000 (Dark Yellow).
        RGBQUAD{ 0x00, 0xA1, 0x00, 0x00 },  // #00a100 (Green).
        RGBQUAD{ 0x00, 0xFF, 0x00, 0x00 },  // #00ff00 (Lime).
        RGBQUAD{ 0x9D, 0xFF, 0xA0, 0x00 },  // #a0ff9d (Light Green).
        RGBQUAD{ 0x9B, 0x00, 0x00, 0x00 },  // #00009b (Dark Blue).
        RGBQUAD{ 0xFF, 0x00, 0x00, 0x00 },  // #0000ff (Blue).
        RGBQUAD{ 0xFF, 0x00, 0xA2, 0x00 },  // #a200ff (Purple).
        RGBQUAD{ 0xFF, 0x00, 0xFF, 0x00 },  // #ff00ff (Pink/Magenta).
        RGBQUAD{ 0xFF, 0xFF, 0x00, 0x00 },  // #00ffff (Cyan).
        RGBQUAD{ 0x9F, 0xA1, 0xA2, 0x00 },  // #a2a19f (Gray).
        RGBQUAD{ 0xFF, 0xFF, 0xFF, 0x00 },  // #ffffff (White).
    };

    // Update file headers for 4-bit depth.
    const auto row_size{
        static_cast<std::uint32_t>(std::ceil(constants::target_bitcount * bmp_info_header.biWidth / 32) * 4)
    };
    const auto pixel_array_size{ row_size * bmp_info_header.biHeight };
    bmp_file_header.bfSize = pixel_array_size + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmp_file_header.bfOffBits += sizeof(RGBQUAD) * std::pow(2, constants::target_bitcount);
    bmp_info_header.biBitCount = constants::target_bitcount;
    output_file.write(reinterpret_cast<char *>(&bmp_file_header), sizeof(BITMAPFILEHEADER));
    output_file.write(reinterpret_cast<char *>(&bmp_info_header), sizeof(BITMAPINFOHEADER));
    output_file.write(reinterpret_cast<const char *>(palette.data()), sizeof(palette));

    for(size_t row{}; row < bmp_info_header.biHeight; ++row) {
        for(size_t column{}; column < bmp_info_header.biWidth; column += 2) {
            std::pair<RGBTRIPLE, RGBTRIPLE> pixels;
            input_file.read(reinterpret_cast<char *>(&pixels), sizeof(pixels));
            const std::byte two_pixels{
                (utils::find_closest_color(pixels.first, palette) << 4) |
                (utils::find_closest_color(pixels.second, palette))
            };
            output_file.write(reinterpret_cast<const char *>(&two_pixels), sizeof(two_pixels));
        }
    }
}

}  // namespace setm::bmp

int main() {
    using namespace setm::bmp;

    // Convert input 24-bit BMP to 4-bit.
    convert_bmp_24_to_4_depth(constants::input_bmp_file_path, constants::output_bmp_file_path);
}
