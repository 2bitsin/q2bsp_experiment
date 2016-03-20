//
//  wal.hpp
//  Bsp_Take0
//
//  Created by Aleksandras Sevcenko on 3/20/16.
//  Copyright © 2016 Aleksandr Ševčenko. All rights reserved.
//

#ifndef wal_hpp
#define wal_hpp

#include <cstdint>
#include <cstddef>

#include "bsp_structs.hpp"
#include "array_view.hpp"
#include "bitmap.hpp"

namespace xtk {
    struct wal_bitmap {
        bitmap miptex [4];
        std::string name;
        std::string next_name;
    };
    
    
    wal_bitmap wal_decode (const array_view<std::uint8_t>& data, const array_view<bitmap::value_type>& colormap);

}

#endif /* wal_hpp */
