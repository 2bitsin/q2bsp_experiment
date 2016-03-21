//
//  wal.cpp
//  Bsp_Take0
//
//  Created by Aleksandras Sevcenko on 3/20/16.
//  Copyright © 2016 Aleksandr Ševčenko. All rights reserved.
//

#include "wal.hpp"
#include "bsp.hpp"

using namespace xtk ;


static bitmap make_bitmap_level (const array_view<std::uint8_t>& data, const array_view<bitmap::value_type>& colormap, int level, const wal_header& header) {

    auto div1 = 1 << level;
    
    auto width = header.width / div1 ;
    auto height = header.height / div1 ;
    
    auto buffer = std::make_unique<bitmap::value_type[]> (width*height);
    
    auto data_view = array_view<std::uint8_t> {
        data.begin () + header.offset [level],
        data.begin () + header.offset [level] + width*height
    };
    
    for (auto i = 0; i < width*height; ++i) {
        buffer [i] = colormap [data_view [i]];
    }
    
    return bitmap (std::move (buffer), width, height);
};

wal_bitmap xtk::wal_decode (const array_view<std::uint8_t>& data, const array_view<bitmap::value_type>& colormap) {
    const auto& header = *(const wal_header*)data.data ();
    
    auto _bitmap = wal_bitmap {
        make_bitmap_level (data, colormap, 0, header),
        make_bitmap_level (data, colormap, 1, header),
        make_bitmap_level (data, colormap, 2, header),
        make_bitmap_level (data, colormap, 3, header),
        std::string (header.name,
            strnlen (header.name,
                sizeof (header.name))),
        std::string (header.next_name,
            strnlen (header.next_name,
                sizeof (header.next_name)))
    };
    
    return std::move (_bitmap);
}