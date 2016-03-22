//
//  bsp.hpp
//  Bsp_Take0
//
//  Created by Aleksandras Sevcenko on 3/17/16.
//  Copyright © 2016 Aleksandr Ševčenko. All rights reserved.
//

#ifndef bsp_hpp
#define bsp_hpp

#include <vector>

#include "bsp_structs.hpp"
#include "array_view.hpp"
#include "pcx.hpp"
#include "wal.hpp"
#include "q2pak.hpp"


namespace xtk {
    
    struct bsp_data {
        struct vertex_attribute {
    
            inline vertex_attribute (
                const bsp_point3f& _1 = bsp_point3f (),
                const bsp_point3f& _2 = bsp_point3f (),
                const bsp_point3f& _3 = bsp_point3f (),
                const bsp_point3f& _4 = bsp_point3f (),
                const bsp_point3f& _5 = bsp_point3f ())
            noexcept:
                vertex (_1),
                normal (_2),
                uvtex  (_3),
                color  (_4),
                uvlmap (_5)
            {}
            
            bsp_point3f vertex;
            bsp_point3f normal;
            bsp_point3f uvlmap;
            bsp_point3f color;
            bsp_point3f uvtex;
        };
        
        std::vector<vertex_attribute> vertexes;
        
        bitmap textures;
        bitmap lightmaps;
    };
    
    bsp_data bsp_decode (q2pak& pak, const std::string& name);
    
}

#endif /* bsp_hpp */
