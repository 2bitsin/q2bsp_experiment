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
#include <fstream>

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

static bsp_point2f uv_from_face (const bsp_point3f& vec, const bsp_texinfo& texinfo) {
    return bsp_point2f(
        glm::dot (vec, texinfo.u_axis) + texinfo.u_offset,
        glm::dot (vec, texinfo.v_axis) + texinfo.v_offset
    );
}

void stretch (xtk::bitmap& _output, int _oid, const xtk::bitmap& _input, int _iid = 0) {
    auto dx = 1.0f / _output.width ();
    auto dy = 1.0f / _output.height ();

    for (auto y = 0; y < _output.height (); ++y) {
        for (auto x = 0; x < _output.width (); ++x) {
            _output (x, y, _oid) = _input (x*dx, y*dy, _iid);
        }
    }
}

void etch_lightmap (xtk::bitmap& _output, int x0, int y0, const array_view<glm::tvec3<std::uint8_t>>& lmview, std::int32_t w_lmap, std::int32_t h_lmap) {
    const auto _input = tbitmap<std::uint8_t, glm::tvec3> (lmview, w_lmap, h_lmap);
	auto dx = 1.0f/16.0f;
    auto dy = 1.0f/16.0f;
	
	for (auto y = 0; y < 16; ++y) {
		for (auto x = 0; x < 16; ++x) {
			_output (x0 + x, y0 + y, 0) = bitmap::value_type (_input (dx*x, dy*y, 0), 255);
		}
	}

 /*
    for (auto y = 0; y < h_lmap; ++y) {
        for (auto x = 0; x < w_lmap; ++x) {
            _output (x0 + x, y0 + y, 0)
            = bitmap::value_type (lmview [w_lmap*y + x], 255);
        }
    }
 */
}

std::uint32_t next_pot (std::uint32_t v) {
    std::uint32_t t = 1;
    while (t < v) {
        t <<= 1;
    }
    return t;
}


bsp_data xtk::bsp_decode (q2pak& pak, const std::string& name) {
    static const auto _white = bsp_point3f (1.0f, 1.0f, 1.0f);

//    xtk::Debug::log ("bsp_decode :\n");
    
    bsp_data _odata;
    
    q2pak_lock _mbspdata_lock (pak, name);
//    xtk::Debug::log ("\tLoading map %s ... \n",  name.c_str ());
    q2pak_lock _colormap_lock (pak, "pics/colormap.pcx");
//    xtk::Debug::log ("\tLoading map %s ... \n", "pics/colormap.pcx");
    
    auto colormap = pcx_decode(*_colormap_lock);
    
    const auto& header = *(const bsp_header*)_mbspdata_lock->data();
    
    auto vertexes        = array_view_from_lump<bsp_point3f>  (*_mbspdata_lock, header.vertices);
    auto edges           = array_view_from_lump<bsp_edge2s>   (*_mbspdata_lock, header.edges);
    auto faces           = array_view_from_lump<bsp_face>     (*_mbspdata_lock, header.faces);
    auto face_edge_list  = array_view_from_lump<std::int32_t> (*_mbspdata_lock, header.face_edge_list);
    auto planes          = array_view_from_lump<bsp_plane>    (*_mbspdata_lock, header.planes);
    auto texture_info    = array_view_from_lump<bsp_texinfo>  (*_mbspdata_lock, header.texinfo);
    auto lightmaps       = array_view_from_lump<std::uint8_t> (*_mbspdata_lock, header.lightmaps);
    auto entities        = array_view_from_lump<char>         (*_mbspdata_lock, header.entities);
    
    auto& buffer = _odata.vertexes;
    
    std::vector<bsp_data::vertex_attribute> temp;
    
    std::unordered_map<std::string, std::int32_t> texture_name_lookup;
    std::vector<wal_bitmap> texture_bits;
    
    auto w_max = 0;
    auto h_max = 0;
    
    auto palette = array_view<bitmap::value_type> {
        colormap.data () + colormap.width ()*(0),
		colormap.data () + colormap.width ()*(0+1)
    };
    
    /****************************************************************/
    /* FIRST PASS: loading wal textures and figuring out parameters */
    /****************************************************************/
    
    for (const auto& _tex : texture_info) {
        
        auto name = string_from_array (_tex.texture_name);
        
        auto texit = texture_name_lookup.find (name);
        if (texit != texture_name_lookup.end ()) {
            continue;
        }
        
        q2pak_lock texlock (pak, "textures/" + name + ".wal");
        
        texture_name_lookup.emplace (name, (std::int32_t)texture_bits.size ());
        texture_bits.emplace_back (wal_decode (*texlock, palette));
        
        const auto& tex = texture_bits.back ();
        
        w_max = std::max (w_max, tex.miptex [0].width ());
        h_max = std::max (h_max, tex.miptex [0].height ());
        
        xtk::Debug::log ("\tLoading texuture %s (%u * %u) ...\n", name.c_str (), tex.miptex [0].width (), tex.miptex [0].height ());
    }
    
    /*************************************************/
    /*  SECOND PASS: preparing 2D texture array data */
    /*************************************************/
    
    auto lmedgew = 16*next_pot ((std::uint32_t)std::sqrt ((float)faces.length()));
    
    _odata.textures  = bitmap (w_max, h_max, (std::int32_t)texture_bits.size ());
    _odata.lightmaps = bitmap (lmedgew, lmedgew, 1);
    
    for (const auto& _tex : texture_info) {
        auto name = string_from_array (_tex.texture_name);
        
        auto texit = texture_name_lookup.find (name);
        if (texit == texture_name_lookup.end ()) {
            xtk::Debug::log("%s has disapeared somewhere...\n", name.c_str ());
            continue;
        }
        
        stretch (_odata.textures, texit->second, texture_bits [texit->second].miptex [0], 1);
        
    }
    xtk::Debug::log("Processing faces ");
    for (auto face_index = 0; face_index < faces.length(); ++face_index) {
        const auto& face = faces [face_index];
        
        /*****************************************************/
        /* Gathering texture info                            */
        /*****************************************************/
        
        const auto& texinfo = texture_info [face.texture_info];
        
        auto texit = texture_name_lookup.find (string_from_array (texinfo.texture_name));
        
        auto texid = -1.0f;
        auto texw = 0.015625f;
        auto texh = 0.015625f;
        
        if (texit != texture_name_lookup.end ()) {
            const auto& texmip = texture_bits [texit->second].miptex [0];
            
            texid = (float)texit->second;
            
            texw = 1.0f/texmip.width ();
            texh = 1.0f/texmip.height ();
        }
        
        auto _tex_uv_mul = glm::vec3 (texw, texh, 1.0f);
        
        /////////////////////////////////////////////////////////
        
        auto normal = face.plane_side
            ? -planes [face.plane].normal
            : +planes [face.plane].normal;
        
        temp.reserve (face.num_edges);
        
        auto avg = bsp_point3f (0.0f);
        auto div = 0.5f/face.num_edges;
        
        auto min_u = +INFINITY;
        auto max_u = -INFINITY;
        auto min_v = +INFINITY;
        auto max_v = -INFINITY;
        
        for (auto i = 0; i < face.num_edges; ++i) {
            
            auto edge = face_edge_list [face.first_edge + i];
            
            auto vx0 = vertexes [edge < 0 ? edges [-edge] [1] : edges [+edge] [0]] ;
            auto vx1 = vertexes [edge < 0 ? edges [-edge] [0] : edges [+edge] [1]] ;
            
            auto uv0 = bsp_point3f (uv_from_face (vx0, texinfo), texid);
            auto uv1 = bsp_point3f (uv_from_face (vx1, texinfo), texid);
            
            temp.emplace_back (vx0, normal, uv0, _white, bsp_point2f (0.0f));
            temp.emplace_back (vx1, normal, uv1, _white, bsp_point2f (0.0f));
            
            avg += vx0;
            avg += vx1;
            
            min_u = std::min (min_u, std::floor (uv0.x));
            max_u = std::max (max_u, std::floor (uv0.x));
            
            min_v = std::min (min_v, std::floor (uv0.y));
            max_v = std::max (max_v, std::floor (uv0.y));
            
            min_u = std::min (min_u, std::floor (uv1.x));
            max_u = std::max (max_u, std::floor (uv1.x));
            
            min_v = std::min (min_v, std::floor (uv1.y));
            max_v = std::max (max_v, std::floor (uv1.y));
        }
        
        auto auv = bsp_point3f (uv_from_face (avg *= div, texinfo), texid);
        temp.emplace_back (avg, normal, auv, _white, bsp_point2f (0.0f));
        
        
        /****************************************/
        /* Gather lightmap data                 */
        /****************************************/
        
        if (reinterpret_cast<const std::uint32_t&>(face.lightmap_syles)) {
    
            auto w_lmap = int (std::ceil (max_u / 16) - std::floor (min_u / 16) + 1);
            auto h_lmap = int (std::ceil (max_v / 16) - std::floor (min_v / 16) + 1);
        
            //__xtk_assert(std::out_of_range, w_lmap <= 16 && h_lmap <= 16);
    
            auto lightmap_view = array_view<glm::tvec3<std::uint8_t>>  {
                (const glm::tvec3<std::uint8_t>*)(lightmaps.begin () + face.lightmap_offset),
                (const glm::tvec3<std::uint8_t>*)(lightmaps.begin () + face.lightmap_offset) + w_lmap*h_lmap
            };
            
            auto lmx0 = (face_index % (lmedgew >> 4)) << 4;
            auto lmy0 = (face_index / (lmedgew >> 4)) << 4;
            
            auto delta_edge = 1.0f/lmedgew;
            
            etch_lightmap (_odata.lightmaps, lmx0, lmy0, lightmap_view, w_lmap, h_lmap);
        
            
            for (auto i = 0; i < temp.size (); ++i) {
				
				auto lu = 16.0f*((temp [i].uvtex.x - min_u)/(w_lmap*16.0f));
				auto lv = 16.0f*((temp [i].uvtex.y - min_v)/(h_lmap*16.0f));
				__xtk_assert (std::logic_error, lu >= 0.0f && lu < 16.0f);
				__xtk_assert (std::logic_error, lv >= 0.0f && lv < 16.0f);
			 
			 /*
				auto lu = ((temp [i].uvtex.x - min_u)/(16.0f));
				auto lv = ((temp [i].uvtex.y - min_v)/(16.0f));
			  */
                temp [i].uvlmap = delta_edge * bsp_point2f (lmx0 + lu, lmy0 + lv);
			  
			}
        }
		
		for (auto i = 0; i < temp.size (); ++i) {
			temp [i].uvtex = _tex_uv_mul * temp [i].uvtex;
		}
		
		
        xtk::Debug::log(".");
        
        /****************************************/
        /* Construct face polygons              */
        /****************************************/
        
        
        for (auto i = 0; i < temp.size () - 1; ++i) {
            buffer.emplace_back (temp [i]);
            if (i & 1) {
                buffer.emplace_back (temp.back ());
            }
        }
        
        temp.clear ();
    }
    
    xtk::Debug::log(" done.\n");
    
    
	
	_odata.lightmaps.write_as_tga("lightmaps.tga");
	
    return std::move (_odata);
}






