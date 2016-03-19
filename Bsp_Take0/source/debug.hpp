//
//  debug.hpp
//  Bsp_Take0
//
//  Created by Aleksandras Sevcenko on 3/17/16.
//  Copyright © 2016 Aleksandr Ševčenko. All rights reserved.
//

#ifndef debug_h
#define debug_h

#include <cstdio>

namespace xtk {
    struct Debug {
        template <typename... Args>
        static void log (Args&&... args) {
            std::printf (args...);
        }
    };
}

#endif /* debug_h */
