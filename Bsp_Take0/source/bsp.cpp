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

template <typename T>
inline array_view<T> array_view_from_lump (const xtk::array_view<std::uint8_t>& data, const bsp_lump& lump) {
    return {
        reinterpret_cast<const T*> (data.data() + lump.offset),
        reinterpret_cast<const T*> (data.data() + lump.offset + lump.length)
    };
}

template <std::size_t _Count>
static std::string string_from_array (const char (&array) [_Count]) {
    return std::string (array, strnlen(array, sizeof (array)));
}

static bsp_point2f uv_from_face (const bsp_point3f& vec, const bsp_texinfo& texinfo, float texw, float texh) {
    return bsp_point2f(
        texw*(glm::dot (vec, texinfo.u_axis) + texinfo.u_offset),
        texh*(glm::dot (vec, texinfo.v_axis) + texinfo.v_offset)
    );
}

bsp_data xtk::bsp_decode (pakman& pak, const std::string& name) {
    xtk::Debug::log ("bsp_decode :\n");
    
    bsp_data _odata;
    
    pakman_lock _mbspdata_lock (pak, name);
//    xtk::Debug::log ("\tLoading map %s ... \n",  name.c_str ());
    pakman_lock _colormap_lock (pak, "pics/colormap.pcx");
//    xtk::Debug::log ("\tLoading map %s ... \n", "pics/colormap.pcx");
    
    auto colormap = pcx_decode(*_colormap_lock);
    
    const auto& header = *(const bsp_header*)_mbspdata_lock->data();
    
    auto vertexes        = array_view_from_lump<bsp_point3f> (*_mbspdata_lock, header.vertices);
    auto edges           = array_view_from_lump<bsp_edge2s>  (*_mbspdata_lock, header.edges);
    auto faces           = array_view_from_lump<bsp_face>    (*_mbspdata_lock, header.faces);
    auto face_edge_list  = array_view_from_lump<std::int32_t>(*_mbspdata_lock, header.face_edge_list);
    auto planes          = array_view_from_lump<bsp_plane>   (*_mbspdata_lock, header.planes);
    auto texture_info    = array_view_from_lump<bsp_texinfo> (*_mbspdata_lock, header.texinfo);
    
    auto& buffer = _odata.vertexes;
    
    std::unordered_map<std::string, std::uint32_t> loaded_textures;
    
    auto palette = array_view<bitmap::value_type> {
        colormap.data (), colormap.data () + colormap.width ()
    };
    
    auto counter = 0u;
    
    for (const auto& _tex : texture_info) {
        ++counter;
        
        auto name = string_from_array (_tex.texture_name);
        
        auto texit = loaded_textures.find (name);
        
        if (texit != loaded_textures.end ()) {
            continue;
        }
        
        pakman_lock texlock (pak, "textures/" + name + ".wal");
        _odata.textures.emplace_back (wal_decode (*texlock, palette));
        
        const auto& tex = _odata.textures.back ();
//        xtk::Debug::log ("\tLoading texuture %s (%u * %u) ...\n", name.c_str (), tex.miptex [0].width(), tex.miptex [0].height ());
        
        loaded_textures.insert({name, _odata.textures.size () - 1});

    }
    
    const auto _white = bsp_point3f (1.0f, 1.0f, 1.0f);
    std::vector<bsp_data::vertex_attribute> temp;

    for (const auto& face: faces) {
    
        const auto& texinfo = texture_info [face.texture_info];
            
        auto texit = loaded_textures.find (string_from_array (texinfo.texture_name));
        auto texid = -1.0f;
        auto texw = 1.0f/64.0f;
        auto texh = 1.0f/64.0f;
        
        if (texit != loaded_textures.end ()) {
            texid = (float)texit->second;
            const auto& texmip = _odata.textures [texit->second].miptex [0];
            texw = 1.0f/texmip.width ();
            texh = 1.0f/texmip.height ();
        }
        
        auto normal = face.plane_side
            ? -planes [face.plane].normal
            : +planes [face.plane].normal;
        
        temp.reserve (face.num_edges);
        
        auto avg = bsp_point3f (0.0f);
        auto div = 1.0f/(face.num_edges * 2.0f);
        
        for (auto i = 0; i < face.num_edges; ++i) {
            
            auto edge = face_edge_list [face.first_edge + i];
            
            auto vx0 = vertexes [edge < 0 ? edges [-edge] [1] : edges [+edge] [0]] ;
            auto vx1 = vertexes [edge < 0 ? edges [-edge] [0] : edges [+edge] [1]] ;
            
            auto uv0 = bsp_point3f (uv_from_face (vx0, texinfo, texw, texh), texid);
            auto uv1 = bsp_point3f (uv_from_face (vx1, texinfo, texw, texh), texid);
            
            temp.emplace_back (vx0, normal, uv0, _white);
            temp.emplace_back (vx1, normal, uv1, _white);
            
            avg += vx0;
            avg += vx1;
        }
        
        auto auv = bsp_point3f (uv_from_face (avg *= div, texinfo, texw, texh), 1.0f);
        
        temp.emplace_back (avg, normal, auv, bsp_point3f {1.0f});
        
        for (auto i = 0; i < temp.size () - 1; ++i) {
            buffer.emplace_back (temp [i]);
            if (i & 1) {
                buffer.emplace_back (temp.back ());
            }
        }
        
        temp.clear ();
        
    }
    
    return std::move (_odata);
}






