//
//  quake2_bsp.hpp
//  Bsp_Take0
//
//  Created by Aleksandras Sevcenko on 3/17/16.
//  Copyright © 2016 Aleksandr Ševčenko. All rights reserved.
//

#ifndef quake2_bsp_hpp
#define quake2_bsp_hpp

#include "bsp.hpp"
#include "bsp_common.hpp"
#include "array_view.hpp"

#include <vector>

namespace xtk {
    
    struct bsp_data_quake2 {
        
        bsp_data_quake2 (std::vector<std::uint8_t> data);
                
    protected:
        std::vector<std::uint8_t> m_Data;
        const quake2::bsp_header& m_Header;
    
    public:
        const array_view<quake2::bsp_point3f> vertexes;
        const array_view<quake2::bsp_edge2s>  edges;
        const array_view<quake2::bsp_face>    faces;
        const array_view<std::int32_t>        face_edge_list;
        const array_view<quake2::bsp_plane>   planes;
        const array_view<quake2::bsp_texinfo> texture_info;
        
    };
    
    struct bsp_vertex_attribute {
        bsp_vertex_attribute (
            const bsp_point3f& _0,
            const bsp_point3f& _1,
            const bsp_point2f& _2)
        noexcept:
            vertex (_0),
            normal (_1),
            uv_tex (_2)
        {}
        
        bsp_point3f vertex;
        bsp_point3f normal;
        bsp_point2f uv_tex;
    };

    void build_bsp_faces (std::vector<bsp_vertex_attribute>& buffer, const bsp_data_quake2& bsp);
}


#endif /* quake2_bsp_hpp */
