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
#include <utility>


namespace fs = std::filesystem;

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

constexpr double color_distance(const rgb_triple &color1, const rgb_quad &color2) noexcept {
    return std::sqrt(std::pow((color2.blue - color1.blue), 2) +
                     std::pow((color2.green - color1.green), 2) +
                     std::pow((color2.red - color1.red), 2));
}

template<std::size_t N>
constexpr std::byte find_closest_color(const rgb_triple &color, const std::array<rgb_quad, N> &palette) noexcept {
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
    bitmap_file_header bmp_file_header;
    input_file.read(reinterpret_cast<char *>(&bmp_file_header), sizeof(bitmap_file_header));
    bitmap_info_header bmp_info_header;
    input_file.read(reinterpret_cast<char *>(&bmp_info_header), sizeof(bitmap_info_header));

    // Check if the file is a BMP file.
    if(bmp_file_header.bf_type != constants::BMP_SIGNATURE) {
        std::cerr << "File " << input_file_path << " is not a BMP file\n";
        return;
    }

    // Check if the number of bits per pixel is 24.
    if(bmp_info_header.bi_bit_count != 24) {
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
        rgb_quad{ 0x00, 0x00, 0x00, 0x00 },  // #000000 (Black).
        rgb_quad{ 0x00, 0x00, 0xFF, 0x00 },  // #ff0000 (Red).
        rgb_quad{ 0x00, 0xA1, 0xFF, 0x00 },  // #ffa100 (Orange).
        rgb_quad{ 0x9F, 0xA0, 0xFF, 0x00 },  // #ffa09f (Light Red).
        rgb_quad{ 0x00, 0xFF, 0xFF, 0x00 },  // #ffff00 (Yellow).
        rgb_quad{ 0x00, 0xA0, 0xA3, 0x00 },  // #a3a000 (Dark Yellow).
        rgb_quad{ 0x00, 0xA1, 0x00, 0x00 },  // #00a100 (Green).
        rgb_quad{ 0x00, 0xFF, 0x00, 0x00 },  // #00ff00 (Lime).
        rgb_quad{ 0x9D, 0xFF, 0xA0, 0x00 },  // #a0ff9d (Light Green).
        rgb_quad{ 0x9B, 0x00, 0x00, 0x00 },  // #00009b (Dark Blue).
        rgb_quad{ 0xFF, 0x00, 0x00, 0x00 },  // #0000ff (Blue).
        rgb_quad{ 0xFF, 0x00, 0xA2, 0x00 },  // #a200ff (Purple).
        rgb_quad{ 0xFF, 0x00, 0xFF, 0x00 },  // #ff00ff (Pink/Magenta).
        rgb_quad{ 0xFF, 0xFF, 0x00, 0x00 },  // #00ffff (Cyan).
        rgb_quad{ 0x9F, 0xA1, 0xA2, 0x00 },  // #a2a19f (Gray).
        rgb_quad{ 0xFF, 0xFF, 0xFF, 0x00 },  // #ffffff (White).
    };

    // Update file headers for 4-bit depth.
    const auto row_size{
        static_cast<std::uint32_t>(std::ceil(constants::target_bitcount * bmp_info_header.bi_width / 32) * 4)
    };
    const auto pixel_array_size{ row_size * bmp_info_header.bi_height };
    bmp_file_header.bf_size = pixel_array_size + sizeof(bitmap_file_header) + sizeof(bitmap_info_header);
    bmp_file_header.bf_off_bits += sizeof(rgb_quad) * std::pow(2, constants::target_bitcount);
    bmp_info_header.bi_bit_count = constants::target_bitcount;
    output_file.write(reinterpret_cast<char *>(&bmp_file_header), sizeof(bitmap_file_header));
    output_file.write(reinterpret_cast<char *>(&bmp_info_header), sizeof(bitmap_info_header));
    output_file.write(reinterpret_cast<const char *>(palette.data()), sizeof(palette));

    for(std::size_t row{}; row < bmp_info_header.bi_height; ++row) {
        for(std::size_t column{}; column < bmp_info_header.bi_width; column += 2) {
            std::pair<rgb_triple, rgb_triple> pixels;
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
