//
//  pakman.cpp
//  Bsp_Take0
//
//  Created by Aleksandras Sevcenko on 3/20/16.
//  Copyright © 2016 Aleksandr Ševčenko. All rights reserved.
//

#include "pakman.hpp"
#include "debug.hpp"

#include <string>

#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>

using namespace xtk;


struct pak_header {
    std::uint32_t signature;
    std::uint32_t offset;
    std::uint32_t length;
};

struct pak_resource {
    char name [56];
    std::uint32_t offset;
    std::uint32_t length;
};

pakman::pakman (std::initializer_list<std::string> args):
    pakman ()
{
    for (const auto& name: args) {
        mount (name);
    }
}

void pakman::mount (const std::string& name, const std::string& prefix) {
    auto it = m_pakfile.find (name);
    if (it != m_pakfile.end ()) {
        return;
    }
    
    struct stat st;
    if (-1 == stat (name.c_str (), &st) || st.st_size <= 0) {
        throw std::runtime_error (name + " not found, or size is 0. "
            "(errno : " + std::to_string (errno) + ")");
    }
    
    auto file_descriptor = open (name.c_str (), O_RDONLY);
    
    if (file_descriptor < 0) {
        throw std::runtime_error (name + ", unable to open file. "
            "(errno : " + std::to_string (errno) + ")");
    }
    
    auto file_pointer = mmap (NULL, st.st_size, PROT_READ, MAP_PRIVATE, file_descriptor, 0);
    
    if (!file_pointer) {
        throw std::runtime_error (name + ", failed mmap (). "
            "(errno : " + std::to_string (errno) + ")");
    }
    
    auto file_view = array_view<std::uint8_t> {
        (std::uint8_t*)file_pointer,
        (std::uint8_t*)file_pointer + st.st_size
    };
    
    m_pakfile.insert ({name, {file_descriptor, file_view}});
    
    const pak_header& header = *(const pak_header*)file_view.begin ();
    
    if (header.signature != 'KCAP') {
        throw std::runtime_error (name + ", not a pack file");
    }
    
    auto directory_view = array_view<pak_resource> {
        (const pak_resource*)(file_view.begin () + header.offset),
        (const pak_resource*)(file_view.begin () + header.offset + header.length)
    };
    
//    xtk::Debug::log("Mounting %s ... size: %llu\n", name.c_str(), (std::uint64_t)st.st_size);
    for (const auto& direntry : directory_view) {
        auto resource_name = std::string (direntry.name, strnlen (direntry.name, 56));
        
        auto resource_view = array_view<std::uint8_t> {
            file_view.begin () + direntry.offset,
            file_view.begin () + direntry.offset + direntry.length
        };
        
        auto id = prefix + resource_name;
        auto it = m_resource.find (id);
        if (it != m_resource.end ()) {
//            xtk::Debug::log ("Warning `%s` already mounted, skipping\n", id.c_str ());
            continue;
        }
        
        m_resource.insert ({id, {0, resource_view}});
//        xtk::Debug::log("\tadding %s ... size: %llu\n", id.c_str (), (std::uint64_t)direntry.length);
    }
}

pakman::~pakman() {
    #if XTK_DEBUG
    for (const auto& resource: m_resource) {
        const auto& name = resource.first;
        const auto& resource_data = resource.second;
        const auto& reference_count = resource_data.first;
        if (reference_count) {
            xtk::Debug::log ("Warning : %s is still locked !\n", name.c_str ());
        }
    }
    #endif
    for (auto& file: m_pakfile) {
//        const auto& name = file.first;
        const auto& file_data = file.second;
        const auto& file_descriptor = file_data.first;
        const auto& file_view = file_data.second;
        
        munmap ((void *)file_view.begin (), file_view.length ());
        close (file_descriptor);
    }
}


const array_view<std::uint8_t>& pakman::lock (const std::string& name) {
    auto it = m_resource.find (name);
    if (it == m_resource.end ()) {
        throw std::runtime_error (name + ", resource not found.");
    }
    
    auto& resource_data = it->second;
    auto& reference_count = resource_data.first;
    
    ++reference_count;
    
    return resource_data.second;
}

void pakman::unlock (const std::string& name) {
    auto it = m_resource.find (name);
    if (it == m_resource.end ()) {
        throw std::runtime_error (name + ", resource not found.");
    }
    
    auto& resource_data = it->second;
    auto& reference_count = resource_data.first;
    
    --reference_count;
}

pakman& pakman::shared () {
    static pakman _shared;
    return _shared;
}


