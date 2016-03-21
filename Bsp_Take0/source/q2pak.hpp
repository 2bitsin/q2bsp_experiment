//
//  q2pak.hpp
//  Bsp_Take0
//
//  Created by Aleksandras Sevcenko on 3/20/16.
//  Copyright © 2016 Aleksandr Ševčenko. All rights reserved.
//

#ifndef q2pak_hpp
#define q2pak_hpp

#include <unordered_map>
#include "array_view.hpp"

namespace xtk {
    struct q2pak {
        
       ~q2pak ();
       
        static q2pak& shared ();
        
        q2pak (std::initializer_list<std::string> args);
        q2pak () = default;
        q2pak (const q2pak&) = delete;
        q2pak (q2pak&&) = delete;
        q2pak& operator = (const q2pak&) = delete;
        q2pak& operator = (q2pak&&) = delete;
        
        void mount (const std::string& path, const std::string& prefix = "");
        
        const array_view<std::uint8_t>& lock (const std::string&);
        void unlock (const std::string&);
       
    private:
    
        std::unordered_map<std::string, std::pair<int, array_view<std::uint8_t>>> m_pakfile;
        std::unordered_map<std::string, std::pair<int, array_view<std::uint8_t>>> m_resource;
    };
    
    
    struct q2pak_lock {
        typedef array_view<std::uint8_t> view_type;
        
        q2pak_lock (q2pak& man, const std::string& name):
            m_view (man.lock (name)),
            m_name (name),
            m_man (man)
        {
        }
        
       ~q2pak_lock () {
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
        q2pak& m_man;
        
    };
}


#endif /* q2pak_hpp */
