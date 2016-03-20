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

namespace xtk {
    /*
    struct bsp_data_quake2 {
        
        bsp_data_quake2 (std::vector<std::uint8_t> data);
                
    protected:
        std::vector<std::uint8_t> m_Data;
        const bsp_header& m_Header;
    
    public:
        const array_view<bsp_point3f> vertexes;
        const array_view<bsp_edge2s>  edges;
        const array_view<bsp_face>    faces;
        const array_view<std::int32_t>        face_edge_list;
        const array_view<bsp_plane>   planes;
        const array_view<bsp_texinfo> texture_info;
        
    };

    namespace quake2 {
        struct bsp_vertex_attribute {
            bsp_vertex_attribute (
                const bsp_point3f& _0,
                const bsp_point3f& _1,
                const bsp_point2f& _2,
                const bsp_point3f& _3)
            noexcept:
                vertex (_0),
                normal (_1),
                uv_tex (_2),
                color (_3)
            {}
        
            bsp_point3f vertex;
            bsp_point3f normal;
            bsp_point2f uv_tex;
            bsp_point3f color;
        };

        void build_bsp_faces (std::vector<bsp_vertex_attribute>& buffer, const bsp_data_quake2& bsp);
    }
    */
}

#endif /* bsp_hpp */
