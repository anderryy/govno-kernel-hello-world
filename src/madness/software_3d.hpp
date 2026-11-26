#pragma once

#include <windows.h>
#include <cstdint>
#include <vector>

namespace govno::madness {

enum class Render3DMode {
    SPINNING_CUBE = 0,
    RAYTRACED_SPHERE = 1,
    MANDELBROT_ZOOM = 2,
    PLASMA_TUNNEL = 3,
    GRAY_SCOTT_ORBIT = 4,
    DNA_DOUBLE_HELIX = 5
};

class Software3DRenderer {
    int width_ = 360;
    int height_ = 200;
    std::vector<uint32_t> fb_;

    void draw_line(int x0, int y0, int x1, int y1, uint32_t col);
    void fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t col);
    void render_cube_mode(uint64_t tick, bool completed, bool disco_chaos);
    void render_raytracer_mode(uint64_t tick, bool disco_chaos);
    void render_mandelbrot_mode(uint64_t tick, bool disco_chaos);
    void render_plasma_tunnel_mode(uint64_t tick, bool disco_chaos);
    void render_interference_orbit_mode(uint64_t tick, bool disco_chaos);
    void render_dna_helix_mode(uint64_t tick, bool disco_chaos);
    void render_loss_graph(const std::vector<float>& loss_hist);

public:
    Software3DRenderer(int w = 360, int h = 200);
    void render_demoscene_frame(
        uint64_t tick,
        bool completed,
        bool disco_chaos,
        Render3DMode mode,
        const std::vector<float>& loss_hist
    );
    void blit_to_hdc(HDC hdc, int dst_x, int dst_y, int dst_w, int dst_h) const;
};

} // namespace govno::madness
