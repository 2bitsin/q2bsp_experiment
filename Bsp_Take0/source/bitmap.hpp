//
//  bitmap.hpp
//  Bsp_Take0
//
//  Created by Aleksandras Sevcenko on 3/20/16.
//  Copyright © 2016 Aleksandr Ševčenko. All rights reserved.
//

#ifndef bitmap_h
#define bitmap_h

#include <glm/glm.hpp>

#include <memory>
#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <algorithm>
#include <fstream>

#include "array_view.hpp"
#include "debug.hpp"

namespace xtk {

    template <typename T, template <typename, glm::precision = glm::precision::highp> typename V = glm::tvec4>
    struct tbitmap {
        typedef V<T> value_type;
        typedef std::unique_ptr<value_type []> data_pointer_type;
        
        const value_type* data () const noexcept {
            return m_data.get ();
        };
        
        auto width  () const noexcept { return m_width; }
        auto height () const noexcept { return m_height; }
        auto depth  () const noexcept { return m_depth; }
        
        auto area   () const noexcept { return width () * height (); }
        auto volume () const noexcept { return area () * depth (); }
        
        
        const value_type& operator () (std::int32_t x, std::int32_t y, std::int32_t z = 0) const noexcept {
            return locate (x, y, z);
        }
        
        value_type& operator () (std::int32_t x, std::int32_t y, std::int32_t z = 0) noexcept {
            return locate (x, y, z);
        }
        
        value_type operator () (float x, float y, std::int32_t z = 0) const noexcept {
            return sample (x*m_width, y*m_height, z);
        }
        
        tbitmap (data_pointer_type ptr, std::int32_t width, std::int32_t height, std::int32_t depth = 1):
            m_data   (std::move (ptr)),
            m_width  (width),
            m_height (height),
            m_depth  (depth)
        {}
        
        tbitmap (): tbitmap (nullptr, 0, 0, 0) {}
        
        tbitmap (std::int32_t width, std::int32_t height, std::int32_t depth = 1):
            tbitmap (std::make_unique<value_type []>(width*height*depth),
                     width, height, depth)
        {}
        
        tbitmap (const array_view<value_type>& view, std::int32_t width, std::int32_t height, std::int32_t depth = 1):
            tbitmap (std::make_unique<value_type []>(width*height*depth),
                     width, height, depth)
        {
            std::copy (view.begin (), view.begin () + width*height*depth, m_data.get ());
        }
        
        tbitmap (const tbitmap<T, V>& _old):
            tbitmap (std::make_unique<value_type []>(_old.volume ()),
                     _old.width(), _old.height(), _old.depth())
        {
            std::copy (_old.data (), _old.data () + _old.volume (), m_data.get ());
        }
        
        tbitmap (tbitmap<T, V>&& _old):
            tbitmap (std::move (_old.m_data),
                std::exchange (_old.m_width, 0),
                std::exchange (_old.m_height, 0),
                std::exchange (_old.m_depth, 0))
        {
        }
        
        tbitmap<T, V>& operator = (const tbitmap<T, V>& _old) {
            m_data   = std::make_unique<value_type []> (_old.volume ());
            m_width  = _old.width ();
            m_height = _old.height ();
            m_depth  = _old.depth ();
            std::copy (_old.data (), _old.data () + _old.volume (), m_data.get ());
            return *this;
        }
        
        tbitmap<T, V>& operator = (tbitmap<T, V>&& _old) {
            m_data   = std::move (_old.m_data);
            m_width  = std::exchange (_old.m_width, 0);
            m_height = std::exchange (_old.m_height, 0);
            m_depth  = std::exchange (_old.m_depth, 0);
            return *this;
        }
		
		void write_as_tga (const std::string& path, int z = 0) {
			typedef std::uint8_t BYTE;
			typedef std::uint16_t WORD;
		#pragma pack(push, 1)
			typedef struct _TgaHeader
			{
				BYTE IDLength;        /* 00h  Size of Image ID field */
				BYTE ColorMapType;    /* 01h  Color map type */
				BYTE ImageType;       /* 02h  Image type code */
				WORD CMapStart;       /* 03h  Color map origin */
				WORD CMapLength;      /* 05h  Color map length */
				BYTE CMapDepth;       /* 07h  Depth of color map entries */
				WORD XOffset;         /* 08h  X origin of image */
				WORD YOffset;         /* 0Ah  Y origin of image */
				WORD Width;           /* 0Ch  Width of image */
				WORD Height;          /* 0Eh  Height of image */
				BYTE PixelDepth;      /* 10h  Image pixel size */
				BYTE ImageDescriptor; /* 11h  Image descriptor byte */
			} TGAHEAD;
		#pragma pack(pop)
			TGAHEAD header = {0};
			header.ImageType = 2;
			header.Width = m_width;
			header.Height = m_height;
			header.PixelDepth = 24;
			header.ImageDescriptor = 0x20;
			std::ofstream out (path, std::ios::binary);
			out.write (reinterpret_cast<const char*>(&header), sizeof (header));
			auto offs = z*area ();
			for (auto i = 0; i < volume(); ++i) {
				out.write (reinterpret_cast<const char *>(&m_data [i + offs].b), sizeof (m_data [i + offs].b));
				out.write (reinterpret_cast<const char *>(&m_data [i + offs].g), sizeof (m_data [i + offs].g));
				out.write (reinterpret_cast<const char *>(&m_data [i + offs].r), sizeof (m_data [i + offs].r));
			}

		}
		
        
    private:
        template <typename A, typename B, typename Q>
        inline static auto _lerp (const A& a, const B& b, const Q& q) noexcept {
            return a*(Q(1) - q) + q*b;
        }
    
        template <typename X>
        inline static auto _wrap (X x, X w) noexcept {
            return (x < 0 ? w + x % w : x) % w;
        }
        
        value_type sample (float x, float y, std::int32_t z) const noexcept {
            
            auto x0 = std::floor (x);
            auto y0 = std::floor (y);
            auto x1 = std::ceil (x);
            auto y1 = std::ceil (y);
            
            auto qx = x - x0;
            auto qy = y - y0;
            
            typedef V<float> fvec;
            
            auto sy0x0 = fvec (locate (x0, y0, z));
            auto sy0x1 = fvec (locate (x1, y0, z));
            auto sy1x0 = fvec (locate (x0, y1, z));
            auto sy1x1 = fvec (locate (x1, y1, z));
            
            return _lerp (_lerp (sy0x0, sy0x1, qx),
                          _lerp (sy1x0, sy1x1, qx),
                          qy);
        }
        
        const value_type& locate (std::int32_t x, std::int32_t y, std::int32_t z) const noexcept {
            x = _wrap<std::int32_t> (x, m_width);
            y = _wrap<std::int32_t> (y, m_height);
            z = _wrap<std::int32_t> (z, m_depth);
            return m_data [x + y*m_width];
        }
    
        value_type& locate (std::int32_t x, std::int32_t y, std::int32_t z) noexcept {
            x = _wrap<std::int32_t> (x, m_width);
            y = _wrap<std::int32_t> (y, m_height);
            z = _wrap<std::int32_t> (z, m_depth);
            return m_data [x + y*m_width + z*m_width*m_height];
        }
    
    
        data_pointer_type m_data;
        std::int32_t m_width;
        std::int32_t m_height;
        std::int32_t m_depth;
    };
    
    typedef tbitmap<std::uint8_t, glm::tvec4> bitmap;
}

#endif /* bitmap_h */
