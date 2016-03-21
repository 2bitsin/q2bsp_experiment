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
    
    #define XTK_DEBUG 1
        
    #if XTK_DEBUG
        #define __xtk_assert(Y, X) if (!(X)) {\
            __builtin_trap ();\
            throw Y ("failed assertion: " #X);\
        }
        #define __xtk_noexcept_if_nodebug
    #else
        #define __xtk_assert(Y, X)
        #define __xtk_noexcept_if_nodebug noexcept
    #endif
    
}

#endif /* debug_h */
