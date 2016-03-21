//
//  pakman.hpp
//  Bsp_Take0
//
//  Created by Aleksandras Sevcenko on 3/20/16.
//  Copyright © 2016 Aleksandr Ševčenko. All rights reserved.
//

#ifndef pakman_hpp
#define pakman_hpp

#include <unordered_map>
#include "array_view.hpp"

namespace xtk {
    struct pakman {
        
       ~pakman ();
       
        static pakman& shared ();
        
        pakman (std::initializer_list<std::string> args);
        pakman () = default;
        pakman (const pakman&) = delete;
        pakman (pakman&&) = delete;
        pakman& operator = (const pakman&) = delete;
        pakman& operator = (pakman&&) = delete;
        
        void mount (const std::string& path, const std::string& prefix = "");
        
        const array_view<std::uint8_t>& lock (const std::string&);
        void unlock (const std::string&);
       
    private:
    
        std::unordered_map<std::string, std::pair<int, array_view<std::uint8_t>>> m_pakfile;
        std::unordered_map<std::string, std::pair<int, array_view<std::uint8_t>>> m_resource;
    };
    
    
    struct pakman_lock {
        typedef array_view<std::uint8_t> view_type;
        
        pakman_lock (pakman& man, const std::string& name):
            m_view (man.lock (name)),
            m_name (name),
            m_man (man)
        {
        }
        
       ~pakman_lock () {
            m_man.unlock (m_name);
        }
        
        const view_type* operator -> () const {
            return &m_view;
        }
        
        const view_type& operator * () const {
            return m_view;
        }
        
    private:
        view_type m_view;
        std::string m_name;
        pakman& m_man;
        
    };
}


#endif /* pakman_hpp */
