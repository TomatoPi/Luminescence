/*
* TESTS DE REFACTORING
*/
#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <cstddef>

namespace mycelium {

    struct control_t {
        using name_t    = std::string;
        using value_t   = std::vector<std::byte>;

        name_t  name;
        value_t value;
    };

    using device_t = std::unordered_set<control_t>;
    
}

template<>
struct std::hash<mycelium::control_t> {
    std::size_t operator() (const mycelium::control_t& ctrl) const noexcept {
        return std::hash<std::string>{}(ctrl.name);
    }
};