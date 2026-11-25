#pragma once

#include <string>

namespace govno::madness {

struct MazeSolveResult {
    std::string delivered_payload;
    size_t nodes_expanded = 0;
    size_t path_length = 0;
    std::string ascii_map_dump;
};

MazeSolveResult route_payload_through_astar_maze(const std::string& payload);

} // namespace govno::madness
