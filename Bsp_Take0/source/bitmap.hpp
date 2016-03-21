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

#include "debug.hpp"

namespace xtk {
    struct bitmap {
        typedef glm::tvec4<std::uint8_t> value_type;
        typedef std::unique_ptr<value_type []> data_pointer_type;
        
        bitmap (data_pointer_type ptr, std::int32_t width, std::int32_t height):
            m_data (std::move (ptr)),
            m_width (width),
            m_height (height)
        {}
        
        const value_type* data () const noexcept {
            return m_data.get ();
        };
        
        auto width () const noexcept { return m_width; }
        auto height () const noexcept { return m_height; }
        
        
        const value_type& operator () (std::int32_t x, std::int32_t y) const noexcept {
            return locate (x, y);
        }
        
        value_type& operator () (std::int32_t x, std::int32_t y) noexcept {
            return locate (x, y);
        }
        
        value_type operator () (float x, float y) const noexcept {
            return sample (x*m_width, y*m_height);
        }
        
    private:
        template <typename A, typename B, typename Q>
        inline static auto _lerp (const A& a, const B& b, const Q& q) noexcept {
            return a*(Q(1) - q) + q*b;
        }
    
        template <typename T>
        inline static auto _wrap (T x, T w) noexcept {
            return (x < 0 ? w + x % w : x) % w;
        }
        
        value_type sample (float x, float y) const noexcept {
            
            auto x0 = std::floor (x);
            auto y0 = std::floor (y);
            auto x1 = std::ceil (x);
            auto y1 = std::ceil (y);
            
            auto qx = x - x0;
            auto qy = y - y0;
            
            auto sy0x0 = glm::vec4 (locate (x0, y0));
            auto sy0x1 = glm::vec4 (locate (x1, y0));
            auto sy1x0 = glm::vec4 (locate (x0, y1));
            auto sy1x1 = glm::vec4 (locate (x1, y1));
            
            return _lerp (_lerp (sy0x0, sy0x1, qx),
                          _lerp (sy1x0, sy1x1, qx),
                          qy);
        }
        
        const value_type& locate (std::int32_t x, std::int32_t y) const noexcept {
            x = _wrap<std::int32_t> (x, m_width);
            y = _wrap<std::int32_t> (y, m_height);
            return m_data [x + y*m_width];
        }
    
        value_type& locate (std::int32_t x, std::int32_t y) noexcept {
            x = _wrap<std::int32_t> (x, m_width);
            y = _wrap<std::int32_t> (y, m_height);
            return m_data [x + y*m_width];
        }
    
    
        data_pointer_type m_data;
        std::int32_t m_width;
        std::int32_t m_height;
    };
}

#endif /* bitmap_h */
