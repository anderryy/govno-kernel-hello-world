#include "madness/maze_astar.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <queue>
#include <random>
#include <sstream>
#include <vector>

namespace govno::madness {

MazeSolveResult route_payload_through_astar_maze(const std::string& payload) {
    constexpr int W = 25;
    constexpr int H = 11;
    std::array<std::array<char, W>, H> grid{};
    for (auto& row : grid) row.fill('#');

    std::mt19937 rng(42069);

    // Carve solvable maze via DFS
    std::vector<std::pair<int, int>> stack;
    grid[1][1] = '.';
    stack.push_back({1, 1});
    const int dirs[4][2] = {{2, 0}, {-2, 0}, {0, 2}, {0, -2}};

    while (!stack.empty()) {
        auto [cy, cx] = stack.back();
        std::vector<int> valid;
        for (int d = 0; d < 4; ++d) {
            int ny = cy + dirs[d][0];
            int nx = cx + dirs[d][1];
            if (ny > 0 && ny < H - 1 && nx > 0 && nx < W - 1 && grid[ny][nx] == '#') {
                valid.push_back(d);
            }
        }
        if (valid.empty()) {
            stack.pop_back();
            continue;
        }
        int pick = valid[rng() % valid.size()];
        int my = cy + dirs[pick][0] / 2;
        int mx = cx + dirs[pick][1] / 2;
        int ny = cy + dirs[pick][0];
        int nx = cx + dirs[pick][1];
        grid[my][mx] = '.';
        grid[ny][nx] = '.';
        stack.push_back({ny, nx});
    }

    // Add extra loops so A* actually searches forks
    for (int k = 0; k < 10; ++k) {
        int ry = 1 + static_cast<int>(rng() % (H - 2));
        int rx = 1 + static_cast<int>(rng() % (W - 2));
        grid[ry][rx] = '.';
    }

    const int sy = 1, sx = 1;
    const int gy = H - 2, gx = W - 2;
    grid[sy][sx] = '.';
    grid[gy][gx] = '.';

    // Run real A* search
    struct Node {
        int y, x, f;
        bool operator>(const Node& o) const { return f > o.f; }
    };
    std::array<std::array<int, W>, H> g_score{};
    std::array<std::array<int, W>, H> parent_y{};
    std::array<std::array<int, W>, H> parent_x{};
    for (auto& r : g_score) r.fill(1000000);
    for (auto& r : parent_y) r.fill(-1);
    for (auto& r : parent_x) r.fill(-1);

    auto heur = [&](int y, int x) { return std::abs(gy - y) + std::abs(gx - x); };
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> pq;
    g_score[sy][sx] = 0;
    pq.push({sy, sx, heur(sy, sx)});

    size_t expanded = 0;
    const int d4[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

    while (!pq.empty()) {
        Node cur = pq.top();
        pq.pop();
        ++expanded;
        if (cur.y == gy && cur.x == gx) break;
        for (int d = 0; d < 4; ++d) {
            int ny = cur.y + d4[d][0];
            int nx = cur.x + d4[d][1];
            if (ny < 0 || ny >= H || nx < 0 || nx >= W || grid[ny][nx] == '#') continue;
            int ng = g_score[cur.y][cur.x] + 1;
            if (ng < g_score[ny][nx]) {
                g_score[ny][nx] = ng;
                parent_y[ny][nx] = cur.y;
                parent_x[ny][nx] = cur.x;
                pq.push({ny, nx, ng + heur(ny, nx)});
            }
        }
    }

    // Reconstruct shortest path and place letters along path
    std::vector<std::pair<int, int>> path;
    int cy = gy, cx = gx;
    while (cy != -1 && cx != -1) {
        path.push_back({cy, cx});
        if (cy == sy && cx == sx) break;
        int py = parent_y[cy][cx];
        int px = parent_x[cy][cx];
        cy = py; cx = px;
    }
    std::reverse(path.begin(), path.end());

    for (const auto& [py, px] : path) grid[py][px] = '*';
    grid[sy][sx] = 'S';
    grid[gy][gx] = 'E';

    std::ostringstream map_ss;
    map_ss << "=== A* DUNGEON MAZE ROUTER (Payload Escaped Labyrinth) ===\r\n"
           << "A* Nodes Expanded: " << expanded << " | Path Steps: " << path.size() << "\r\n\r\n";
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) map_ss << grid[y][x];
        map_ss << "\r\n";
    }
    map_ss << "\r\nDelivered safely across maze to VGA port: \"" << payload << "\"\r\n";

    return {payload, expanded, path.size(), map_ss.str()};
}

} // namespace govno::madness
