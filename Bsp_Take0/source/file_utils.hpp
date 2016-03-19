//
//  file_utils.hpp
//  Bsp_Take0
//
//  Created by Aleksandras Sevcenko on 3/17/16.
//  Copyright © 2016 Aleksandr Ševčenko. All rights reserved.
//

#ifndef file_utils_hpp
#define file_utils_hpp

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <string>
#include <utility>
#include <iomanip>

namespace xtk {

    inline std::size_t probe_stream_size (std::istream&& iss) {
        auto s0 = iss.tellg ();
        iss.seekg (0u, std::ios::end);
        auto s1 = iss.tellg ();
        iss.seekg (s0, std::ios::beg);
        return s1;
    }

    template <typename T = std::uint8_t>
    auto fetch_data (std::istream&& iss, std::size_t how_much) {
        std::vector<T> data;
        data.resize (how_much);
        iss.read(reinterpret_cast<char*>(std::data (data)), how_much * sizeof (T));
        data.resize (iss.gcount () / sizeof (T));
        return data;
    }

    template <typename T = std::uint8_t>
    auto fetch_data (std::istream&& iss) {
        return fetch_data (std::move (iss), probe_stream_size (std::move (iss)));
    }

    template <typename Iter>
    void dump_hex (Iter a, Iter b, std::ostream&& oss = std::move (std::cout), std::size_t n = 16u) {
        typedef typename std::iterator_traits<Iter>::value_type value_type;
        const auto value_digits = sizeof (value_type)*2u;
    
        std::string last_line (n, ' ');
    
        auto counter = 0u;
        for (auto i = a; i != b; ++i) {
            if (counter % n == 0u) {
                oss << std::endl
                << std::hex
                << std::setfill ('0')
                << std::setw (64/4)
                << counter
                << " : ";
            }
            oss << std::setw (value_digits) << std::uint64_t (*i) << " ";
            last_line [counter % n] = ((*i) >= 32 && (*i) < 128 ? char (*i) : '.');
            ++counter;
            if (counter % n == 0u) {
                oss << " | " << last_line;
            }
        }
    }
}

#endif /* file_utils_hpp */
