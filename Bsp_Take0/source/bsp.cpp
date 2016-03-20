//
//  quake2_bsp.cpp
//  Bsp_Take0
//
//  Created by Aleksandras Sevcenko on 3/17/16.
//  Copyright © 2016 Aleksandr Ševčenko. All rights reserved.
//

#include "bsp.hpp"
#include "debug.hpp"

#include <set>
#include <memory>
#include <vector>
#include <cstdio>
#include <iostream>

using namespace xtk;

/*
template <typename T>
inline array_view<T> _array_view_from_lump (const std::vector<std::uint8_t>& data, const bsp_lump& lump) {
    return {
        reinterpret_cast<const T*> (std::data (data) + lump.offset),
        reinterpret_cast<const T*> (std::data (data) + lump.offset + lump.length)
    };
}


bsp_data_quake2::bsp_data_quake2 (std::vector<std::uint8_t> data):
    m_Data          (std::move (data)),
    m_Header        (reinterpret_cast<const bsp_header&> (*std::data (m_Data))),
    vertexes        (_array_view_from_lump<bsp_point3f>(m_Data, m_Header.vertices)),
    edges           (_array_view_from_lump<bsp_edge2s>(m_Data, m_Header.edges)),
    faces           (_array_view_from_lump<bsp_face>(m_Data, m_Header.faces)),
    face_edge_list  (_array_view_from_lump<std::int32_t>(m_Data, m_Header.face_edge_list)),
    planes          (_array_view_from_lump<bsp_plane>(m_Data, m_Header.planes)),
    texture_info    (_array_view_from_lump<bsp_texinfo>(m_Data, m_Header.texinfo))
{
    typedef xtk::bsp_header header_type;
    
    if (m_Header.signature != 'PSBI' || m_Header.version != bsp_version_quake2) {
        Debug::log ("%s : %s", __PRETTY_FUNCTION__, "Not Quake2 compatible BSP (Invalid signature or version)");
        return;
    }
    
    auto total_lump_volume = 0u;
    
    for (const auto& lump : m_Header.all_lumps) {
        Debug::log ("lump [%u] (offset: %u, length: %u)\n",
                    &lump - &m_Header.all_lumps [0],
                    lump.offset,
                    lump.length);
        total_lump_volume += lump.length;
    }
    
    Debug::log (
        "    size of header : %llu,\n"
        " total lump volume : %llu,\n"
        "   total file size : %llu.\n",
        std::uint64_t (sizeof (m_Header)),
        std::uint64_t (total_lump_volume),
        std::uint64_t (m_Data.size ())
    );
    
    Debug::log ("          Vertexes : %llu\n"
                "             Edges : %llu\n"
                "             Faces : %llu\n"
                "Face/edge elements : %llu\n"
                "            Planes : %llu\n",
                (std::uint64_t)vertexes.length (),
                (std::uint64_t)edges.length (),
                (std::uint64_t)faces.length (),
                (std::uint64_t)face_edge_list.length (),
                (std::uint64_t)planes.length ());
    
    
    if (auto missMatch = m_Data.size () - total_lump_volume - sizeof (m_Header)) {
        Debug::log ("WARNING: Total lump volume mismatch by number of bytes: %lld\n", std::int64_t (missMatch));
    }
}

static quake2::bsp_point2f uv_from_face (const quake2::bsp_point3f& vec, const quake2::bsp_texinfo& texinfo) {
    return {
        vec.x * texinfo.u_axis.x + vec.y * texinfo.u_axis.y + vec.z * texinfo.u_axis.z + texinfo.u_offset,
        vec.x * texinfo.v_axis.x + vec.y * texinfo.v_axis.y + vec.z * texinfo.v_axis.z + texinfo.v_offset
    };
}

static quake2::bsp_point3f face_normal (bsp_point3f normal, std::uint16_t side) {
    if (side) {
        normal.x = -normal.x;
        normal.y = -normal.y;
        normal.z = -normal.z;
    }
    return normal; 
}

void xtk::quake2::build_bsp_faces (std::vector<bsp_vertex_attribute>& buffer, const bsp_data_quake2& bsp) {
    quake2::bsp_point3f color_table [] = {
        {1.0f, 1.0f, 1.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 1.0f},
        {1.0f, 0.5f, 0.5f},
        {0.5f, 1.0f, 0.5f},
        {0.5f, 0.5f, 1.0f},
        {1.0f, 1.0f, 0.5f},
        {1.0f, 0.5f, 1.0f},
        {0.5f, 1.0f, 1.0f},
        {0.5f, 0.0f, 0.0f},
        {0.0f, 0.5f, 0.0f},
        {0.0f, 0.0f, 0.5f},
        {0.5f, 0.5f, 0.0f},
        {0.5f, 0.0f, 0.5f},
        {0.0f, 0.5f, 0.5f}
    };
    
    
    std::vector<xtk::quake2::bsp_vertex_attribute> temp;
    //std::set<std::pair<std::uint32_t, std::int32_t>> seen;
    
    unsigned counter = 0u;
    for (const auto& face: bsp.faces) {
        ++counter;

        const auto& texinfo = bsp.texture_info [face.texture_info];
        auto normal = face_normal (bsp.planes [face.plane].normal, face.plane_side);
        
        temp.reserve (face.num_edges);
        
        quake2::bsp_point3f average = {0.0f, 0.0f, 0.0f};
        
        const auto& _color = color_table [face.texture_info % std::size (color_table)];
       
        for (auto i = 0; i < face.num_edges; ++i) {
            auto edge = bsp.face_edge_list [face.first_edge + i];
            
            auto vx0 = bsp.vertexes [edge < 0 ? bsp.edges [-edge] [1] : bsp.edges [+edge] [0]] ;
            auto vx1 = bsp.vertexes [edge < 0 ? bsp.edges [-edge] [0] : bsp.edges [+edge] [1]] ;
            
            auto uv0 = uv_from_face (vx0, texinfo);
            auto uv1 = uv_from_face (vx1, texinfo);
            
            temp.emplace_back (vx0, normal, uv0, _color);
            temp.emplace_back (vx1, normal, uv1, _color);
            
            average.x += vx0.x + vx1.x;
            average.y += vx0.y + vx1.y;
            average.z += vx0.z + vx1.z;
        }
        
        auto divisor = 1.0f/(face.num_edges*2.0f);
        
        average.x *= divisor;
        average.y *= divisor;
        average.z *= divisor;
        auto avguv = uv_from_face (average, texinfo);
        
        temp.emplace_back (average, normal, avguv, _color);
        
        for (auto i = 0; i < temp.size () - 1; ++i) {
            buffer.emplace_back (temp [i]);
            if (i & 1) {
                buffer.emplace_back (temp.back ());
            }
        }
        
        temp.clear ();
    }
}

*/


