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
#include <stdexcept>

#include "debug.hpp"

namespace xtk {
    struct bitmap {
        typedef glm::tvec4<std::uint8_t> value_type;
        typedef std::unique_ptr<value_type []> data_pointer_type;
        
        bitmap (data_pointer_type ptr, std::size_t width, std::size_t height):
            m_data (std::move (ptr)),
            m_width (width),
            m_height (height)
        {}
        
        const value_type* data () const noexcept {
            return m_data.get ();
        };
        
        auto width () const noexcept { return m_width; }
        auto height () const noexcept { return m_height; }
        
        const value_type& operator () (std::size_t x,
                                       std::size_t y)
            const __xtk_noexcept_if_nodebug
        {
            __xtk_assert (std::out_of_range, x < m_width && y < m_height);
            return m_data [x + y*m_width];
        }
        
    private:
        data_pointer_type m_data;
        std::size_t m_width;
        std::size_t m_height;
    };
}

#endif /* bitmap_h */
