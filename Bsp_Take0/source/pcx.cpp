//
//  pcx.cpp
//  Bsp_Take0
//
//  Created by Aleksandras Sevcenko on 3/20/16.
//  Copyright © 2016 Aleksandr Ševčenko. All rights reserved.
//

#include "pcx.hpp"
#include "debug.hpp"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <memory>

using namespace xtk;

#pragma pack(push, 1)

struct pcx_header {
    std::uint8_t    vendor_id;
    std::uint8_t    version;
    std::uint8_t    encoding;
    std::uint8_t    bits_per_pixel;
    std::uint16_t   xmin;
    std::uint16_t   ymin;
    std::uint16_t   xmax;
    std::uint16_t   ymax;
    std::uint16_t   vertical_dpi;
    std::uint8_t    palette [48];
    std::uint8_t    reserved0;
    std::uint8_t    color_planes;
    std::uint16_t   bytes_per_line;
    std::uint16_t   palette_type;
    std::uint16_t   horizontal_source_size;
    std::uint16_t   vertical_source_size;
    std::uint8_t    filler [56];
};

#pragma pack(pop)

bitmap xtk::pcx_decode (const array_view<std::uint8_t>& data) {

    static_assert (sizeof (pcx_header) == 128, "Header length incorrect");

    const auto& header = *(const pcx_header*)data.data();
    
    auto width  = header.xmax - header.xmin + 1;
    auto height = header.ymax - header.ymin + 1;
    
    __xtk_assert (std::invalid_argument, header.vendor_id == 0x0A && header.version >= 5);
    
    xtk::Debug::log(
        "load_pcx: \n"
        "\tversion = %d\n"
        "\tbits_per_pixel = %d\n"
        "\tcolor_planes = %d\n"
        "\twidth = %u\n"
        "\theight = %u\n"
        ,
        header.version,
        header.bits_per_pixel,
        header.color_planes,
        width,
        height
    );
    
    __xtk_assert(std::invalid_argument,
        header.bits_per_pixel == 8 &&
        header.color_planes == 0);
    
    auto buffer = std::make_unique<bitmap::value_type []> (width*height);
    
    auto stream = array_view<const std::uint8_t> {data.begin() + sizeof (header), data.end()};

    auto next_pixel = 0u;
    auto next_sbyte = stream.begin ();
        
    auto palette = xtk::array_view<glm::tvec3<std::uint8_t>> {
        (const glm::tvec3<std::uint8_t> *)data.end () - 256,
        (const glm::tvec3<std::uint8_t> *)data.end ()
    };
    
    while (next_sbyte < stream.end ()) {
        auto rle = *next_sbyte;
        
        ++next_sbyte;
        
        if (rle >= 0xc0) {
            rle &= 0x3f;
            auto pixel = bitmap::value_type (palette [*next_sbyte], 255);
            ++next_sbyte;
            for (auto j = 0; j < rle; ++j) {
                buffer [next_pixel] = pixel;
                ++next_pixel;
                __xtk_assert (std::overflow_error, next_pixel <= width*height);
            }
        }
        else {
            buffer [next_pixel] = bitmap::value_type (palette [rle], 255);
            ++next_pixel;
            __xtk_assert (std::overflow_error, next_pixel <= width*height);
        }
        
        if (next_pixel >= width*height) {
            break;
        }
    }
    
    __xtk_assert (std::invalid_argument, next_pixel == width*height);
    
    return bitmap (std::move (buffer), width, height);
}
