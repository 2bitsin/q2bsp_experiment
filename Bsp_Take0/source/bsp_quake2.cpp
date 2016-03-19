//
//  quake2_bsp.cpp
//  Bsp_Take0
//
//  Created by Aleksandras Sevcenko on 3/17/16.
//  Copyright © 2016 Aleksandr Ševčenko. All rights reserved.
//

#include "bsp_quake2.hpp"
#include "bsp_common.hpp"
#include "debug.hpp"
#include <memory>
#include <vector>
#include <cstdio>

using namespace xtk;

template <typename T>
inline array_view<T> _array_view_from_lump (const std::vector<std::uint8_t>& data, const bsp_lump& lump) {
    return {reinterpret_cast<const T*> (std::data (data) + lump.offset),
            reinterpret_cast<const T*> (std::data (data) + lump.offset + lump.length)
            };
}


bsp_data_quake2::bsp_data_quake2 (std::vector<std::uint8_t> data):
    m_Data          (std::move (data)),
    m_Header        (reinterpret_cast<const quake2::bsp_header&> (*std::data (m_Data))),
    vertexes        (_array_view_from_lump<quake2::bsp_point3f>(m_Data, m_Header.vertices)),
    edges           (_array_view_from_lump<quake2::bsp_edge2s>(m_Data, m_Header.edges)),
    faces           (_array_view_from_lump<quake2::bsp_face>(m_Data, m_Header.faces)),
    face_edge_list  (_array_view_from_lump<std::int32_t>(m_Data, m_Header.face_edge_list)),
    planes          (_array_view_from_lump<quake2::bsp_plane>(m_Data, m_Header.planes)),
    texture_info    (_array_view_from_lump<quake2::bsp_texinfo>(m_Data, m_Header.texinfo))
{
    typedef xtk::quake2::bsp_header header_type;
    
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

void build_bsp_faces (std::vector<bsp_vertex_attribute>& buffer, const bsp_data_quake2& bsp) {
    for (const auto& face: bsp.faces) {
        auto e0 = face.first_edge;
        auto e1 = face.first_edge + face.num_edges;
        
        auto last_edge = -1;
        
        for (auto i = e0; i < e1; ++i) {
            auto edge = bsp.face_edge_list [i];
            
            auto edge_v0 = 0u;
            auto edge_v1 = 0u;
            
            if (edge < 0) {
                edge = -edge;
                edge_v0 = bsp.edges [edge] [1];
                edge_v1 = bsp.edges [edge] [0];
            }
            else {
                edge_v0 = bsp.edges [edge] [0];
                edge_v1 = bsp.edges [edge] [1];
            }
            
            last_edge = edge_v1;
            
            auto n0 = bsp.planes [face.plane].normal;
            
            if (face.plane_side) {
                n0.x = -n0.x;
                n0.y = -n0.y;
                n0.z = -n0.z;
            }
            
            const auto& texinfo = bsp.texture_info [face.texture_info];
            
            //if (last_edge != edge_v0) {
                auto v0 = bsp.vertexes [edge_v0];
            
                auto uv0 = bsp_point2f {
                    v0.x * texinfo.u_axis.x + v0.y * texinfo.u_axis.y + v0.z * texinfo.u_axis.z + texinfo.u_offset,
                    v0.x * texinfo.v_axis.x + v0.y * texinfo.v_axis.y + v0.z * texinfo.v_axis.z + texinfo.v_offset
                };
            
                buffer.emplace_back (v0, n0, uv0);
            //}
            
            auto v1 = bsp.vertexes [edge_v1];
            
            auto uv1 = bsp_point2f {
                v1.x * texinfo.u_axis.x + v1.y * texinfo.u_axis.y + v1.z * texinfo.u_axis.z + texinfo.u_offset,
                v1.x * texinfo.v_axis.x + v1.y * texinfo.v_axis.y + v1.z * texinfo.v_axis.z + texinfo.v_offset
            };
            
            buffer.emplace_back (v1, n0, uv1);
            
        }
    }
}




