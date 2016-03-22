//
//  pcx.hpp
//  Bsp_Take0
//
//  Created by Aleksandras Sevcenko on 3/20/16.
//  Copyright © 2016 Aleksandr Ševčenko. All rights reserved.
//

#ifndef pcx_hpp
#define pcx_hpp

#include "bitmap.hpp"
#include "array_view.hpp"

namespace xtk {
    
    bitmap pcx_decode (const array_view<std::uint8_t>& data);
}

#endif /* pcx_hpp */
