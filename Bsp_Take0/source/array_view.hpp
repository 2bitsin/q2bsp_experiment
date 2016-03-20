//
//  array_view.hpp
//  Bsp_Take0
//
//  Created by Aleksandras Sevcenko on 3/17/16.
//  Copyright © 2016 Aleksandr Ševčenko. All rights reserved.
//

#ifndef array_view_h
#define array_view_h

#include <stdexcept>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>

#include "debug.hpp"

namespace xtk {


    template <typename T>
    struct array_view {
        typedef T value_type;
        typedef T *iterator;
        typedef const T* const_iterator;
        typedef std::ptrdiff_t difference_type;
        typedef std::size_t size_type;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef value_type* pointer;
        typedef const value_type* const_pointer;
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
        
    private:
        iterator m_Begin, m_End;
    public:
        
        
        iterator begin () noexcept { return m_Begin; }
        iterator end () noexcept { return m_End; }
        const_iterator begin () const noexcept { return m_Begin; }
        const_iterator end () const noexcept { return m_End; }
        const_iterator cbegin () const noexcept { return m_Begin; }
        const_iterator cend () const noexcept { return m_End; }
        reverse_iterator rbegin () noexcept { return end ()-1; }
        reverse_iterator rend () noexcept { return begin ()-1; }
        const_reverse_iterator rbegin () const noexcept { return end ()-1; }
        const_reverse_iterator rend () const noexcept { return begin ()-1; }
        const_reverse_iterator crbegin () const noexcept { return end ()-1; }
        const_reverse_iterator crend () const noexcept { return begin ()-1; }
        /*
        array_view (iterator _begin, iterator _end):
            m_Begin (_begin),
            m_End (_end)
        {
            __xtk_assert (std::logic_error, m_End - m_Begin >= 0);
        }
        */
        array_view (const_iterator _begin, const_iterator _end):
            m_Begin (const_cast<iterator> (_begin)),
            m_End (const_cast<iterator> (_end))
        {
            __xtk_assert (std::logic_error, m_End - m_Begin >= 0);
        }
        
        
        array_view (const array_view<T>&) noexcept = default;
        array_view<T>& operator = (const array_view<T>&) noexcept = default;
        
        array_view () noexcept : array_view (iterator (nullptr), iterator (nullptr)) {}
       ~array_view () = default;
        
        size_type size () const noexcept { return m_End - m_Begin; }
        size_type max_size () const noexcept { return size (); }
        size_type length () const noexcept { return size (); }
        bool empty () const noexcept { return length () == 0u; }
        
        iterator data () noexcept { return begin () ; }
        const_iterator data () const noexcept { return begin () ; }
        
        auto& operator [] (std::size_t i) __xtk_noexcept_if_nodebug {
            __xtk_assert (std::out_of_range, i < length ()) ;
            return m_Begin [i] ;
        }
        
        const auto& operator [] (std::size_t i) const __xtk_noexcept_if_nodebug {
            __xtk_assert (std::out_of_range, i < length ()) ;
            return m_Begin [i] ;
        }
        
        auto& at (std::size_t i) {
            if (i < length ())
                return m_Begin [i] ;
            using namespace std::string_literals;
            throw std::out_of_range (__PRETTY_FUNCTION__ + " : Index out of range."s);
        }
        
        const auto& at (std::size_t i) const {
            if (i < length ())
                return m_Begin [i] ;
            using namespace std::string_literals;
            throw std::out_of_range (__PRETTY_FUNCTION__ + " : Index out of range."s);
        }
        
        auto& front () { return at (0u) ; }
        const auto& front () const { return at (0u) ; }
        auto& back () { return at (length () - 1u) ; }
        const auto& back () const { return at (length () - 1u) ; }
        
        void fill (const value_type& value) {
            for (auto& write: *this)
                write = value;
        }
        
        void swap (array_view<value_type>& other) {
            std::swap (
                std::tie (other.m_Begin, other.m_End),
                std::tie (m_Begin, m_End)
            );
        }
        
    };
}



#endif /* array_view_h */
