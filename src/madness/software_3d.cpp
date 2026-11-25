#include "madness/software_3d.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace govno::madness {

namespace {

uint32_t make_rgb(int r, int g, int b) {
    return (static_cast<uint32_t>(std::clamp(r, 0, 255)) << 16) |
           (static_cast<uint32_t>(std::clamp(g, 0, 255)) << 8) |
           static_cast<uint32_t>(std::clamp(b, 0, 255));
}

uint32_t hsv_argb(int hue_deg) {
    int h = ((hue_deg % 360) + 360) % 360;
    int reg = h / 60;
    int rem = (h % 60) * 255 / 60;
    switch (reg) {
        case 0: return make_rgb(255, rem, 0);
        case 1: return make_rgb(255 - rem, 255, 0);
        case 2: return make_rgb(0, 255, rem);
        case 3: return make_rgb(0, 255 - rem, 255);
        case 4: return make_rgb(rem, 0, 255);
        default: return make_rgb(255, 0, 255 - rem);
    }
}

struct Vec3 {
    float x, y, z;
    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
};
float dot3(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vec3 norm3(const Vec3& v) {
    float len = std::sqrt(dot3(v, v));
    return (len > 1e-6f) ? v * (1.0f / len) : Vec3{0, 1, 0};
}

} // namespace

Software3DRenderer::Software3DRenderer(int w, int h)
    : width_(w), height_(h), fb_(static_cast<size_t>(w * h), 0) {}

void Software3DRenderer::draw_line(int x0, int y0, int x1, int y1, uint32_t col) {
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        if (x0 >= 0 && x0 < width_ && y0 >= 0 && y0 < height_) {
            fb_[static_cast<size_t>(y0 * width_ + x0)] = col;
        }
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void Software3DRenderer::fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t col) {
    int min_x = std::max(0, std::min({x0, x1, x2}));
    int max_x = std::min(width_ - 1, std::max({x0, x1, x2}));
    int min_y = std::max(0, std::min({y0, y1, y2}));
    int max_y = std::min(height_ - 1, std::max({y0, y1, y2}));
    auto edge = [](int ax, int ay, int bx, int by, int px, int py) -> int {
        return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
    };
    int area = edge(x0, y0, x1, y1, x2, y2);
    if (area == 0) return;
    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            int w0 = edge(x1, y1, x2, y2, x, y);
            int w1 = edge(x2, y2, x0, y0, x, y);
            int w2 = edge(x0, y0, x1, y1, x, y);
            if ((area > 0 && w0 >= 0 && w1 >= 0 && w2 >= 0) ||
                (area < 0 && w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                fb_[static_cast<size_t>(y * width_ + x)] = col;
            }
        }
    }
}

void Software3DRenderer::render_cube_mode(uint64_t tick, bool completed, bool disco_chaos) {
    float t = static_cast<float>(tick) * (disco_chaos ? 0.14f : 0.06f);

    for (int y = 0; y < height_; ++y) {
        float fy = static_cast<float>(y) * 0.045f;
        for (int x = 0; x < width_; ++x) {
            float fx = static_cast<float>(x) * 0.032f;
            float v = std::sin(fx + t) + std::sin(fy - t * 0.7f) + std::sin((fx + fy) * 0.6f + t * 0.5f);
            if (!completed) {
                int g = static_cast<int>(25 + 22 * (v + 3.0f) / 6.0f);
                fb_[static_cast<size_t>(y * width_ + x)] = make_rgb(8, g, g / 2 + 20);
            } else {
                int hue = static_cast<int>((v + 3.0f) * 60.0f + tick * (disco_chaos ? 12 : 4));
                fb_[static_cast<size_t>(y * width_ + x)] = hsv_argb(hue);
            }
        }
    }

    for (int s = 0; s < 40; ++s) {
        float sx_base = static_cast<float>((s * 97 % 200) - 100);
        float sy_base = static_cast<float>((s * 53 % 120) - 60);
        float z = 1.0f + std::fmod(static_cast<float>(s * 7) + 1000.0f - t * 6.0f, 30.0f);
        if (z < 0.5f) z = 0.5f;
        int px = width_ / 2 + static_cast<int>((sx_base / z) * 10.0f);
        int py = height_ / 2 + static_cast<int>((sy_base / z) * 10.0f);
        if (px >= 1 && px < width_ - 1 && py >= 1 && py < height_ - 1) {
            fb_[static_cast<size_t>(py * width_ + px)] = 0xFFFFFF;
            fb_[static_cast<size_t>(py * width_ + px + 1)] = 0xFFFFFF;
        }
    }

    const std::array<Vec3, 8> verts = {{
        {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
        {-1, -1,  1}, {1, -1,  1}, {1, 1,  1}, {-1, 1,  1}
    }};
    const int faces[6][4] = {
        {0, 1, 2, 3}, {5, 4, 7, 6}, {4, 0, 3, 7},
        {1, 5, 6, 2}, {3, 2, 6, 7}, {4, 5, 1, 0}
    };

    float ax = t * 0.8f, ay = t * 1.1f;
    float cx = std::cos(ax), sx = std::sin(ax), cy = std::cos(ay), sy = std::sin(ay);
    std::array<Vec3, 8> tv{};
    std::array<int, 8> px{}, py{};
    int ccx = completed ? (width_ * 2 / 3) : (width_ / 2);
    int ccy = height_ / 2;

    for (size_t i = 0; i < 8; ++i) {
        Vec3 v = verts[i];
        float x1 = v.x * cy + v.z * sy;
        float z1 = -v.x * sy + v.z * cy;
        float y2 = v.y * cx - z1 * sx;
        float z2 = v.y * sx + z1 * cx;
        tv[i] = {x1, y2, z2 + 4.0f};
        float inv_z = 110.0f / tv[i].z;
        px[i] = ccx + static_cast<int>(x1 * inv_z);
        py[i] = ccy + static_cast<int>(y2 * inv_z);
    }

    struct FaceDraw { int idx; float avg_z; };
    std::array<FaceDraw, 6> order{};
    for (int f = 0; f < 6; ++f) {
        order[f] = {f, 0.25f * (tv[faces[f][0]].z + tv[faces[f][1]].z + tv[faces[f][2]].z + tv[faces[f][3]].z)};
    }
    std::sort(order.begin(), order.end(), [](const FaceDraw& a, const FaceDraw& b) { return a.avg_z > b.avg_z; });

    for (const auto& fd : order) {
        int f = fd.idx;
        int i0 = faces[f][0], i1 = faces[f][1], i2 = faces[f][2], i3 = faces[f][3];
        if (completed) {
            uint32_t col = hsv_argb(f * 58 + static_cast<int>(tick * 4));
            fill_triangle(px[i0], py[i0], px[i1], py[i1], px[i2], py[i2], col);
            fill_triangle(px[i0], py[i0], px[i2], py[i2], px[i3], py[i3], col);
        }
        uint32_t wc = completed ? 0x000000 : 0x00FF66;
        draw_line(px[i0], py[i0], px[i1], py[i1], wc);
        draw_line(px[i1], py[i1], px[i2], py[i2], wc);
        draw_line(px[i2], py[i2], px[i3], py[i3], wc);
        draw_line(px[i3], py[i3], px[i0], py[i0], wc);
    }
}

void Software3DRenderer::render_raytracer_mode(uint64_t tick, bool disco_chaos) {
    // REAL-TIME CPU RAYTRACER: reflective chrome sphere + hard shadow + infinite checkerboard!
    float t = static_cast<float>(tick) * (disco_chaos ? 0.14f : 0.07f);
    const Vec3 cam{0.0f, 0.35f, -2.5f};
    const Vec3 sphere_c{0.55f + std::sin(t) * 0.45f, 0.12f + std::abs(std::sin(t * 1.6f)) * 0.45f, 0.35f + std::cos(t) * 0.3f};
    const float R2 = 0.60f * 0.60f;
    const Vec3 light_dir = norm3({0.6f, 1.0f, -0.5f});
    const float floor_y = -0.55f;

    auto sample_sky_or_floor = [&](const Vec3& ro, const Vec3& rd) -> uint32_t {
        if (rd.y < -0.001f) {
            float tf = (floor_y - ro.y) / rd.y;
            if (tf > 0.0f && tf < 40.0f) {
                Vec3 hit = ro + rd * tf;
                int cx = static_cast<int>(std::floor(hit.x * 1.8f + t * 0.6f));
                int cz = static_cast<int>(std::floor(hit.z * 1.8f + t * 1.2f));
                bool chk = ((cx ^ cz) & 1) != 0;

                // Shadow ray toward light
                Vec3 oc = hit - sphere_c;
                float b = dot3(oc, light_dir);
                float c = dot3(oc, oc) - R2;
                float disc = b * b - c;
                bool in_shadow = (disc > 0.0f && -b > 0.0f);

                float fade = std::exp(-tf * 0.08f);
                int r = chk ? 245 : 25;
                int g = chk ? 245 : 200;
                int bl = chk ? 40 : 255;
                if (in_shadow) { r /= 3; g /= 3; bl /= 3; }
                return make_rgb(static_cast<int>(r * fade), static_cast<int>(g * fade), static_cast<int>(bl * fade));
            }
        }
        float sky = std::clamp(rd.y * 0.8f + 0.3f, 0.0f, 1.0f);
        return make_rgb(static_cast<int>(40 + 180 * (1.0f - sky)), static_cast<int>(20 + 140 * sky), static_cast<int>(120 + 135 * sky));
    };

    for (int y = 0; y < height_; ++y) {
        float ny = (0.5f - static_cast<float>(y) / height_) * 1.35f;
        for (int x = 0; x < width_; ++x) {
            float nx = (static_cast<float>(x) / width_ - 0.5f) * 2.4f;
            Vec3 rd = norm3({nx, ny, 1.4f});

            Vec3 oc = cam - sphere_c;
            float b = dot3(oc, rd);
            float c = dot3(oc, oc) - R2;
            float disc = b * b - c;
            if (disc > 0.0f) {
                float ts = -b - std::sqrt(disc);
                if (ts > 0.01f) {
                    Vec3 hit = cam + rd * ts;
                    Vec3 n = norm3(hit - sphere_c);
                    Vec3 refl = rd - n * (2.0f * dot3(rd, n));
                    uint32_t env = sample_sky_or_floor(hit, refl);
                    float spec = std::pow(std::max(0.0f, dot3(refl, light_dir)), 24.0f);
                    float diff = std::max(0.15f, dot3(n, light_dir));
                    int er = static_cast<int>((env >> 16) & 0xFF);
                    int eg = static_cast<int>((env >> 8) & 0xFF);
                    int eb = static_cast<int>(env & 0xFF);
                    int r = static_cast<int>(er * 0.72f + 255 * spec + 60 * diff);
                    int g = static_cast<int>(eg * 0.72f + 255 * spec + 20 * diff);
                    int bl = static_cast<int>(eb * 0.80f + 255 * spec + 90 * diff);
                    fb_[static_cast<size_t>(y * width_ + x)] = make_rgb(r, g, bl);
                    continue;
                }
            }
            fb_[static_cast<size_t>(y * width_ + x)] = sample_sky_or_floor(cam, rd);
        }
    }
}

void Software3DRenderer::render_loss_graph(const std::vector<float>& loss_hist) {
    if (loss_hist.empty()) return;
    int gx = 8, gy = height_ - 72, gw = 118, gh = 64;
    for (int y = gy; y < gy + gh; ++y) {
        for (int x = gx; x < gx + gw; ++x) fb_[static_cast<size_t>(y * width_ + x)] = 0x081408;
    }
    draw_line(gx, gy, gx, gy + gh - 1, 0x00FF00);
    draw_line(gx, gy + gh - 1, gx + gw - 1, gy + gh - 1, 0x00FF00);

    float max_loss = *std::max_element(loss_hist.begin(), loss_hist.end());
    if (max_loss < 0.01f) max_loss = 0.01f;
    for (size_t i = 1; i < loss_hist.size(); ++i) {
        int x0 = gx + 2 + static_cast<int>(((i - 1) * (gw - 4)) / std::max<size_t>(1, loss_hist.size() - 1));
        int x1 = gx + 2 + static_cast<int>((i * (gw - 4)) / std::max<size_t>(1, loss_hist.size() - 1));
        int y0 = gy + gh - 3 - static_cast<int>(std::clamp(loss_hist[i - 1] / max_loss, 0.0f, 1.0f) * (gh - 6));
        int y1 = gy + gh - 3 - static_cast<int>(std::clamp(loss_hist[i] / max_loss, 0.0f, 1.0f) * (gh - 6));
        draw_line(x0, y0, x1, y1, 0xFFFF00);
    }
}

void Software3DRenderer::render_demoscene_frame(
    uint64_t tick,
    bool completed,
    bool disco_chaos,
    Render3DMode mode,
    const std::vector<float>& loss_hist
) {
    if (mode == Render3DMode::RAYTRACED_SPHERE) {
        render_raytracer_mode(tick, disco_chaos);
    } else {
        render_cube_mode(tick, completed, disco_chaos);
    }
    render_loss_graph(loss_hist);
}

void Software3DRenderer::blit_to_hdc(HDC hdc, int dst_x, int dst_y, int dst_w, int dst_h) const {
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width_;
    bmi.bmiHeader.biHeight = -height_;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    SetStretchBltMode(hdc, COLORONCOLOR);
    StretchDIBits(
        hdc, dst_x, dst_y, dst_w, dst_h,
        0, 0, width_, height_, fb_.data(), &bmi, DIB_RGB_COLORS, SRCCOPY
    );
}

} // namespace govno::madness
