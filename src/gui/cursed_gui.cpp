#include "gui/cursed_gui.hpp"
#include "kernel/govno_kernel.hpp"
#include "holyc/holyc_compiler.hpp"
#include "madness/chiptune_synth.hpp"
#include "madness/software_3d.hpp"
#include "kernel_types.hpp"
#include "cursed_macros.hpp"

#include <commctrl.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace govno::gui {

namespace {

constexpr int IDC_EDIT_LOG        = 1001;
constexpr int IDC_TAB_LOG         = 1101;
constexpr int IDC_TAB_HOLYC_AOT   = 1102;
constexpr int IDC_TAB_HOLYC_SRC   = 1103;
constexpr int IDC_TAB_HOLYC_VM    = 1104;
constexpr int IDC_TAB_ASTAR       = 1105;
constexpr int IDC_TAB_ML_GA       = 1106;
constexpr int IDC_TAB_RSA_MORSE   = 1107;
constexpr int IDC_TAB_XMLSQL      = 1108;
constexpr int IDC_TAB_SCRIPT      = 1109;
constexpr int IDC_TAB_TOKENS      = 1110;
constexpr int IDC_TAB_AST         = 1111;
constexpr int IDC_TAB_IR          = 1112;
constexpr int IDC_TAB_LISP_C      = 1113;
constexpr int IDC_TAB_BRAINFUCK   = 1114;
constexpr int IDC_TAB_SUBLEQ      = 1115;
constexpr int IDC_TAB_ASM_NET_POW = 1116;

constexpr int IDC_BTN_REBUILD     = 1201;
constexpr int IDC_BTN_TURBO       = 1202;
constexpr int IDC_BTN_HOLYC_IDE   = 1203;
constexpr int IDC_BTN_MUSIC       = 1204;
constexpr int IDC_BTN_RAYTRACE    = 1205;
constexpr int IDC_BTN_TEMPLEOS    = 1206;
constexpr int IDC_BTN_SWARM       = 1207;
constexpr int IDC_BTN_DISCO       = 1208;
constexpr int IDC_BTN_BSOD        = 1209;
constexpr int IDC_BTN_EXIT        = 1210;

constexpr UINT_PTR TIMER_ID_ANIM  = 42;

enum class ActiveTab {
    LOG = 0,
    HOLYC_AOT,
    HOLYC_SRC,
    HOLYC_VM,
    ASTAR,
    ML_GA,
    RSA_MORSE,
    XMLSQL,
    SCRIPT,
    TOKENS,
    AST,
    IR,
    LISP_C,
    BRAINFUCK,
    SUBLEQ,
    ASM_NET_POW
};

std::wstring utf8_to_wide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int needed = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    if (needed <= 0) return L"";
    std::wstring out(static_cast<size_t>(needed), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), out.data(), needed);
    return out;
}

std::string wide_to_utf8(const std::wstring& w) {
    if (w.empty()) return "";
    int needed = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    if (needed <= 0) return "";
    std::string out(static_cast<size_t>(needed), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), out.data(), needed, nullptr, nullptr);
    return out;
}

COLORREF hsv_neon(int hue_deg) {
    int h = ((hue_deg % 360) + 360) % 360;
    int region = h / 60;
    int rem = (h % 60) * 255 / 60;
    switch (region) {
        case 0: return RGB(255, rem, 0);
        case 1: return RGB(255 - rem, 255, 0);
        case 2: return RGB(0, 255, rem);
        case 3: return RGB(0, 255 - rem, 255);
        case 4: return RGB(rem, 0, 255);
        default: return RGB(255, 0, 255 - rem);
    }
}

struct AppContext {
    HWND hwnd = nullptr;
    HWND edit_log = nullptr;
    std::vector<HWND> tab_buttons;
    std::vector<HWND> action_buttons;
    std::vector<HWND> satellite_windows;

    HFONT font_comic_big = nullptr;
    HFONT font_comic_title = nullptr;
    HFONT font_comic_small = nullptr;
    HFONT font_mono = nullptr;
    HFONT font_ui_ugly = nullptr;
    HFONT font_impact = nullptr;

    HBRUSH brush_teal_bg = nullptr;
    HBRUSH brush_term_bg = nullptr;

    std::atomic<bool> cancel_worker{false};
    std::thread worker;

    govno::madness::ChiptuneSynth synth;
    govno::madness::Software3DRenderer renderer3d{360, 200};
    govno::madness::Render3DMode render3d_mode = govno::madness::Render3DMode::SPINNING_CUBE;

    ActiveTab active_tab = ActiveTab::LOG;
    bool tab_dirty = true;
    size_t last_log_count = 0;
    std::wstring last_rendered_text;

    uint64_t anim_tick = 0;
    bool fake_bsod_mode = false;
    bool disco_chaos_mode = false;
    bool templeos_vga_mode = false;
    bool holyc_ide_editing = false;
    bool swarm_active = false;
    int exit_troll_clicks = 0;
};

AppContext& ctx() {
    static AppContext instance;
    return instance;
}

void destroy_satellite_windows() {
    auto& c = ctx();
    for (HWND w : c.satellite_windows) {
        if (w && IsWindow(w)) DestroyWindow(w);
    }
    c.satellite_windows.clear();
    c.swarm_active = false;
}

void spawn_satellite_windows(HWND parent) {
    auto& c = ctx();
    destroy_satellite_windows();
    const wchar_t* labels[] = {
        L"[ДЕМОН HOLYC #1]\r\n2`3 = 8!",
        L"[A* НАВИГАТОР #2]\r\nИщу выход...",
        L"[RSA-ВЗЛОМЩИК #3]\r\nN = 67591",
        L"[МАЙНЕР ЯДРА #4]\r\n+420 GovnoCoin"
    };
    for (int i = 0; i < 4; ++i) {
        HWND sat = CreateWindowExW(
            WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
            L"STATIC",
            labels[i],
            WS_POPUP | WS_BORDER | WS_VISIBLE | SS_CENTER,
            100 + i * 180, 100, 165, 48,
            parent, nullptr, GetModuleHandleW(nullptr), nullptr
        );
        SendMessageW(sat, WM_SETFONT, reinterpret_cast<WPARAM>(c.font_ui_ugly), TRUE);
        c.satellite_windows.push_back(sat);
    }
    c.swarm_active = true;
}

void update_satellite_orbits(HWND parent) {
    auto& c = ctx();
    if (!c.swarm_active || c.satellite_windows.empty()) return;
    RECT prc{};
    GetWindowRect(parent, &prc);
    int cx = (prc.left + prc.right) / 2;
    int cy = (prc.top + prc.bottom) / 2;
    int rx = (prc.right - prc.left) / 2 + 75;
    int ry = (prc.bottom - prc.top) / 2 + 25;

    for (size_t i = 0; i < c.satellite_windows.size(); ++i) {
        HWND sat = c.satellite_windows[i];
        if (!sat || !IsWindow(sat)) continue;
        double ang = c.anim_tick * 0.055 + static_cast<double>(i) * 1.570796;
        int sx = cx + static_cast<int>(std::cos(ang) * rx) - 82;
        int sy = cy + static_cast<int>(std::sin(ang) * ry) - 24;
        SetWindowPos(sat, HWND_TOPMOST, sx, sy, 165, 48, SWP_NOACTIVATE | SWP_NOSIZE);
    }
}

void set_holyc_ide_mode(bool enable) {
    auto& c = ctx();
    c.holyc_ide_editing = enable;
    if (!c.edit_log) return;
    SendMessageW(c.edit_log, EM_SETREADONLY, enable ? FALSE : TRUE, 0);
    if (enable) {
        const wchar_t* starter =
            L"// === ЖИВАЯ ПЕСОЧНИЦА HOLYC! ОТРЕДАКТИРУЙТЕ И НАЖМИТЕ 'ПУСК HOLYC' ===\r\n"
            L"#exe {\r\n"
            L"  StreamPrint(\"I64 POWER = 2`3;\\n\");\r\n"
            L"}\r\n\r\n"
            L"U0 EmitAscii(U8 ch) {\r\n"
            L"  if (32 <= ch <= 126) {\r\n"
            L"    asm { MOV RAX, ch; INT 0x80; }\r\n"
            L"  }\r\n"
            L"}\r\n\r\n"
            L"U0 Main() {\r\n"
            L"  \"$FG,YELLOW$Привет из моего живого HolyC кода!$FG$\\n\";\r\n"
            L"  GodTalk(4);\r\n"
            L"  I64 x = 64 + POWER;\r\n"
            L"  EmitAscii(x);\r\n"
            L"  EmitAscii(105);\r\n"
            L"}\r\n";
        SetWindowTextW(c.edit_log, starter);
        SetWindowTextW(c.action_buttons[2], L"ПУСК HOLYC!");
    } else {
        SetWindowTextW(c.action_buttons[2], L"HOLYC IDE");
        c.tab_dirty = true;
    }
}

void start_pipeline_thread(bool turbo) {
    auto& c = ctx();
    set_holyc_ide_mode(false);
    c.cancel_worker.store(true);
    if (c.worker.joinable()) c.worker.join();
    c.cancel_worker.store(false);
    {
        std::lock_guard<std::mutex> lock(g_state().mtx);
        g_state().turbo_mode = turbo;
    }
    c.last_log_count = 0;
    c.last_rendered_text.clear();
    c.tab_dirty = true;
    c.fake_bsod_mode = false;

    c.worker = std::thread([turbo]() {
        govno::kernel::run_entire_kernel_and_vm_pipeline(ctx().cancel_worker, turbo);
    });
}

void update_layout(HWND hwnd) {
    auto& c = ctx();
    RECT rc{};
    GetClientRect(hwnd, &rc);
    int w = std::max(960, static_cast<int>(rc.right - rc.left));
    int h = std::max(720, static_cast<int>(rc.bottom - rc.top));

    int right_panel_w = 385;
    int left_w = std::max(460, w - right_panel_w - 30);
    int left_x = 12;

    // 16 tab buttons in 2 rows of 8
    int tab_y0 = 164;
    int tab_h = 22;
    int cols = 8;
    int tab_w = std::max(46, (left_w - (cols - 1) * 3) / cols);
    for (size_t i = 0; i < c.tab_buttons.size(); ++i) {
        int row = static_cast<int>(i / 8);
        int col = static_cast<int>(i % 8);
        MoveWindow(c.tab_buttons[i], left_x + col * (tab_w + 3), tab_y0 + row * (tab_h + 3), tab_w, tab_h, TRUE);
    }

    int bottom_btn_h = 34;
    int canvas_h = 228;
    int bottom_y = h - bottom_btn_h - 10;
    int canvas_y = bottom_y - canvas_h - 8;
    int log_y = tab_y0 + 2 * (tab_h + 3) + 4;
    int log_h = std::max(115, canvas_y - log_y - 8);

    if (c.edit_log) {
        MoveWindow(c.edit_log, left_x, log_y, left_w, log_h, TRUE);
    }

    // 10 bottom buttons
    int btn_count = static_cast<int>(c.action_buttons.size());
    int btn_w = std::max(72, (w - 24 - (btn_count - 1) * 4) / std::max(1, btn_count));
    for (int i = 0; i < btn_count; ++i) {
        MoveWindow(c.action_buttons[i], 12 + i * (btn_w + 4), bottom_y, btn_w, bottom_btn_h, TRUE);
    }
}

void sync_terminal_text() {
    auto& c = ctx();
    if (!c.edit_log || c.holyc_ide_editing) return;

    std::string text_utf8;
    bool is_completed = false;
    bool need_update = c.tab_dirty;

    {
        std::lock_guard<std::mutex> lock(g_state().mtx);
        is_completed = g_state().completed;

        if (c.active_tab == ActiveTab::LOG) {
            if (g_state().kernel_logs.size() != c.last_log_count || c.tab_dirty) {
                need_update = true;
                c.last_log_count = g_state().kernel_logs.size();
                std::ostringstream os;
                for (const auto& ln : g_state().kernel_logs) os << ln << "\r\n";
                text_utf8 = os.str();
            }
        } else if (c.tab_dirty) {
            switch (c.active_tab) {
                case ActiveTab::HOLYC_AOT:   text_utf8 = g_state().artifacts.holyc_aot_dump; break;
                case ActiveTab::HOLYC_SRC:   text_utf8 = g_state().artifacts.holyc_source_dump; break;
                case ActiveTab::HOLYC_VM:    text_utf8 = g_state().artifacts.holyc_vm_dump; break;
                case ActiveTab::ASTAR:       text_utf8 = g_state().artifacts.astar_maze_dump; break;
                case ActiveTab::ML_GA:       text_utf8 = g_state().artifacts.ml_genetics_dump; break;
                case ActiveTab::RSA_MORSE:   text_utf8 = g_state().artifacts.rsa_lz77_morse_dump; break;
                case ActiveTab::XMLSQL:      text_utf8 = g_state().artifacts.xml_json_sql_dump; break;
                case ActiveTab::SCRIPT:      text_utf8 = g_state().artifacts.govnoscript_source; break;
                case ActiveTab::TOKENS:      text_utf8 = g_state().artifacts.tokens_dump; break;
                case ActiveTab::AST:         text_utf8 = g_state().artifacts.ast_dump; break;
                case ActiveTab::IR:          text_utf8 = g_state().artifacts.ir_dump; break;
                case ActiveTab::LISP_C:      text_utf8 = g_state().artifacts.lisp_and_c_dump; break;
                case ActiveTab::BRAINFUCK:
                    text_utf8 =
                        "; === OPTIMIZED BRAINFUCK (" + std::to_string(g_state().artifacts.bf_length) + " chars) ===\r\n" +
                        g_state().artifacts.brainfuck_code +
                        "\r\n\r\n; === VERIFIER OUTPUT ===\r\n\"" +
                        g_state().artifacts.brainfuck_verified_output + "\"\r\n";
                    break;
                case ActiveTab::SUBLEQ:      text_utf8 = g_state().artifacts.subleq_oisc_dump; break;
                case ActiveTab::ASM_NET_POW:
                    text_utf8 =
                        g_state().artifacts.govnoasm_and_linker_dump + "\r\n" +
                        g_state().artifacts.gexe_and_blockchain_dump + "\r\n" +
                        g_state().artifacts.tcp_packets_dump;
                    break;
                default: break;
            }
            if (text_utf8.empty()) {
                text_utf8 = "[ПОДОЖДИТЕ... КОНВЕЙЕР СТРАДАНИЙ ЕЩЁ НЕ ДОШЁЛ ДО ЭТОЙ СТАДИИ]";
            }
        }
    }

    if (!need_update) return;
    c.tab_dirty = false;

    std::wstring wtext = utf8_to_wide(text_utf8);
    if (wtext != c.last_rendered_text) {
        c.last_rendered_text = wtext;
        SetWindowTextW(c.edit_log, wtext.c_str());
        if (c.active_tab == ActiveTab::LOG && !is_completed) {
            int len = GetWindowTextLengthW(c.edit_log);
            SendMessageW(c.edit_log, EM_SETSEL, len, len);
            SendMessageW(c.edit_log, EM_SCROLLCARET, 0, 0);
        }
    }
}

void draw_cursed_progress_bar(
    HDC hdc, int x, int y, int w, int h, const wchar_t* label, int pct, COLORREF fill_col
) {
    auto& c = ctx();
    RECT box{x, y, x + w, y + h};
    HBRUSH bg = CreateSolidBrush(RGB(25, 25, 25));
    FillRect(hdc, &box, bg);
    DeleteObject(bg);
    DrawEdge(hdc, &box, EDGE_SUNKEN, BF_RECT);

    int clamped = std::clamp(pct, 0, 100);
    int fill_w = (w - 4) * clamped / 100;
    if (fill_w > 0) {
        RECT fill_rc{x + 2, y + 2, x + 2 + fill_w, y + h - 2};
        HBRUSH fb = CreateSolidBrush(pct > 100 ? RGB(255, 25, 25) : fill_col);
        FillRect(hdc, &fill_rc, fb);
        DeleteObject(fb);
    }

    SetBkMode(hdc, TRANSPARENT);
    HFONT old_f = static_cast<HFONT>(SelectObject(hdc, c.font_ui_ugly));
    std::wostringstream ws;
    ws << label << L": " << pct << L"%";
    if (pct > 100) ws << L" (ПЕРЕБОР!! ОТКАТ GIT RESET --HARD!)";

    SetTextColor(hdc, RGB(0, 0, 0));
    RECT shadow_rc{x + 7, y + 1, x + w, y + h};
    DrawTextW(hdc, ws.str().c_str(), -1, &shadow_rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SetTextColor(hdc, RGB(255, 255, 255));
    RECT text_rc{x + 6, y, x + w, y + h};
    DrawTextW(hdc, ws.str().c_str(), -1, &text_rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, old_f);
}

void draw_clippy_potato(HDC hdc, int cx, int cy, uint64_t tick, bool cool_glasses, HFONT small_font) {
    int bob = static_cast<int>(std::sin(tick * 0.16) * 4.0);
    int py = cy + bob;

    HBRUSH body_br = CreateSolidBrush(RGB(220, 175, 95));
    HPEN body_pen = CreatePen(PS_SOLID, 2, RGB(50, 20, 0));
    HGDIOBJ old_br = SelectObject(hdc, body_br);
    HGDIOBJ old_pen = SelectObject(hdc, body_pen);
    Ellipse(hdc, cx - 26, py - 32, cx + 26, py + 32);

    HBRUSH white_br = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH black_br = CreateSolidBrush(RGB(0, 0, 0));

    if (cool_glasses) {
        RECT g1{cx - 21, py - 14, cx - 2, py - 1};
        RECT g2{cx + 2, py - 14, cx + 21, py - 1};
        RECT bridge{cx - 2, py - 11, cx + 2, py - 8};
        FillRect(hdc, &g1, black_br);
        FillRect(hdc, &g2, black_br);
        FillRect(hdc, &bridge, black_br);
        RECT glint{cx - 18, py - 12, cx - 12, py - 8};
        FillRect(hdc, &glint, white_br);
    } else {
        SelectObject(hdc, white_br);
        Ellipse(hdc, cx - 17, py - 17, cx - 2, py + 2);
        Ellipse(hdc, cx + 2, py - 17, cx + 17, py + 2);
        int look_x = static_cast<int>(std::sin(tick * 0.09) * 4.0);
        SelectObject(hdc, black_br);
        Ellipse(hdc, cx - 11 + look_x, py - 10, cx - 4 + look_x, py - 3);
        Ellipse(hdc, cx + 6 + look_x, py - 10, cx + 13 + look_x, py - 3);
    }

    Arc(hdc, cx - 13, py + 2, cx + 13, py + 20, cx - 11, py + 13, cx + 11, py + 13);

    RECT bbl{cx + 32, cy - 42, cx + 266, cy + 16};
    HBRUSH bbl_br = CreateSolidBrush(RGB(255, 255, 210));
    FillRect(hdc, &bbl, bbl_br);
    FrameRect(hdc, &bbl, black_br);

    const wchar_t* tips[] = {
        L"Скрепыш: hello_temple.HC скомпилировался прямо при сборке .exe! Зацени 'HOLYC AOT'!",
        L"Скрепыш: Нажми 'РЕЙТРЕЙСИНГ' — там честная трассировка лучей на CPU с отражением!",
        L"Скрепыш: Жми 'HOLYC IDE', пиши свой HolyC с #exe и asm{}, и запускай прямо тут!",
        L"Скрепыш: Кнопка 'РОЙ ОКОН' запускает спутники вокруг окна (осторожно, орбита!).",
    };
    const wchar_t* tip = tips[(tick / 90) % 4];
    HFONT old_f = static_cast<HFONT>(SelectObject(hdc, small_font));
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 0));
    RECT tip_rc{bbl.left + 6, bbl.top + 4, bbl.right - 6, bbl.bottom - 4};
    DrawTextW(hdc, tip, -1, &tip_rc, DT_LEFT | DT_WORDBREAK);

    SelectObject(hdc, old_f);
    SelectObject(hdc, old_br);
    SelectObject(hdc, old_pen);
    DeleteObject(body_br);
    DeleteObject(body_pen);
    DeleteObject(white_br);
    DeleteObject(black_br);
    DeleteObject(bbl_br);
}

void paint_window_double_buffered(HWND hwnd) {
    PAINTSTRUCT ps{};
    HDC screen_hdc = BeginPaint(hwnd, &ps);

    RECT rc{};
    GetClientRect(hwnd, &rc);
    int w = std::max(1, static_cast<int>(rc.right - rc.left));
    int h = std::max(1, static_cast<int>(rc.bottom - rc.top));

    HDC hdc = CreateCompatibleDC(screen_hdc);
    HBITMAP bmp = CreateCompatibleBitmap(screen_hdc, w, h);
    HBITMAP old_bmp = static_cast<HBITMAP>(SelectObject(hdc, bmp));

    auto& c = ctx();

    PipelineSharedState snap;
    {
        std::lock_guard<std::mutex> lock(g_state().mtx);
        snap.stage = g_state().stage;
        snap.status_banner = g_state().status_banner;
        snap.kernel_progress = g_state().kernel_progress;
        snap.ml_crypto_progress = g_state().ml_crypto_progress;
        snap.compiler_progress = g_state().compiler_progress;
        snap.driver_net_progress = g_state().driver_net_progress;
        snap.elapsed_ms = g_state().elapsed_ms;
        snap.completed = g_state().completed;
        snap.ml_loss_history = g_state().ml_loss_history;
        snap.ml_live_prediction = g_state().ml_live_prediction;
        snap.final_vga_output = g_state().final_vga_output;
        snap.cpu = g_state().cpu;
        snap.artifacts.ml_epochs = g_state().artifacts.ml_epochs;
        snap.artifacts.genetic_generations = g_state().artifacts.genetic_generations;
        snap.artifacts.holyc_opcodes_count = g_state().artifacts.holyc_opcodes_count;
        snap.artifacts.holyc_vm_cycles = g_state().artifacts.holyc_vm_cycles;
        snap.artifacts.god_oracle_words = g_state().artifacts.god_oracle_words;
        snap.artifacts.astar_nodes_expanded = g_state().artifacts.astar_nodes_expanded;
        snap.artifacts.subleq_cycles = g_state().artifacts.subleq_cycles;
        snap.artifacts.bf_length = g_state().artifacts.bf_length;
    }

    if (c.disco_chaos_mode) {
        HBRUSH disco_bg = CreateSolidBrush(hsv_neon(static_cast<int>(c.anim_tick * 13)));
        FillRect(hdc, &rc, disco_bg);
        DeleteObject(disco_bg);
    } else {
        FillRect(hdc, &rc, c.brush_teal_bg);
    }

    // Header Banner
    RECT banner_rc{8, 6, w - 8, 70};
    HBRUSH banner_bg = CreateSolidBrush(RGB(255, 255, 0));
    FillRect(hdc, &banner_rc, banner_bg);
    DeleteObject(banner_bg);

    HBRUSH hatch = CreateHatchBrush(HS_DIAGCROSS, RGB(255, 0, 255));
    RECT stripe_l{10, 8, 52, 68};
    RECT stripe_r{w - 52, 8, w - 10, 68};
    FillRect(hdc, &stripe_l, hatch);
    FillRect(hdc, &stripe_r, hatch);
    DeleteObject(hatch);
    DrawEdge(hdc, &banner_rc, EDGE_RAISED, BF_RECT);

    SetBkMode(hdc, TRANSPARENT);
    HFONT old_f = static_cast<HFONT>(SelectObject(hdc, c.font_comic_title));
    const wchar_t* hdr = L"ГОВНО-ЯДРО v5.0: HOLYC AOT + ИИ + RSA + МОРЗЕ + A* + SUBLEQ + РЕЙТРЕЙСИНГ";
    RECT hdr_sh{58, 10, w - 58, 38};
    RECT hdr_fg{56, 8, w - 60, 36};
    SetTextColor(hdc, RGB(0, 255, 255));
    DrawTextW(hdc, hdr, -1, &hdr_sh, DT_CENTER | DT_SINGLELINE);
    SetTextColor(hdc, RGB(215, 0, 0));
    DrawTextW(hdc, hdr, -1, &hdr_fg, DT_CENTER | DT_SINGLELINE);

    SelectObject(hdc, c.font_comic_small);
    std::wstring status_w = utf8_to_wide(snap.status_banner);
    RECT st_rc{56, 40, w - 60, 66};
    SetTextColor(hdc, RGB(0, 0, 140));
    DrawTextW(hdc, status_w.c_str(), -1, &st_rc, DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    // 4 Progress Bars
    int bar_w = std::max(200, w - 24);
    draw_cursed_progress_bar(hdc, 12, 76,  bar_w, 17, L"1. HOLYC AOT + ЯДРО (24 .o)", snap.kernel_progress, RGB(0, 210, 0));
    draw_cursed_progress_bar(hdc, 12, 96,  bar_w, 17, L"2. НЕЙРОСЕТЬ SGD + ГЕНЕТИКА + RSA + МОРЗЕ", snap.ml_crypto_progress, RGB(255, 190, 0));
    draw_cursed_progress_bar(hdc, 12, 116, bar_w, 17, L"3. HOLYC VM + A* + SQL/LISP/C99/BF/SUBLEQ/LD", snap.compiler_progress, RGB(255, 0, 255));
    draw_cursed_progress_bar(hdc, 12, 136, bar_w, 17, L"4. БЛОКЧЕЙН + TCP/IP + ДРАЙВЕРЫ + GOVNOCPU", snap.driver_net_progress, RGB(0, 200, 255));

    // Right Monitor
    int right_panel_w = 385;
    int right_x = w - right_panel_w - 12;
    int bottom_btn_h = 34;
    int canvas_h = 228;
    int bottom_y = h - bottom_btn_h - 10;
    int canvas_y = bottom_y - canvas_h - 8;
    int right_y = 164;
    int right_h = std::max(140, canvas_y - right_y - 8);

    RECT cpu_rc{right_x, right_y, right_x + right_panel_w, right_y + right_h};
    HBRUSH bsod_br = CreateSolidBrush(RGB(0, 0, 145));
    FillRect(hdc, &cpu_rc, bsod_br);
    DeleteObject(bsod_br);
    DrawEdge(hdc, &cpu_rc, EDGE_BUMP, BF_RECT);

    SelectObject(hdc, c.font_mono);
    std::wostringstream cpu_ws;
    cpu_ws << L"== GOVNO + TEMPLEOS MONITOR ==\r\n"
           << L"CPU : " << utf8_to_wide(snap.cpu.cpu_brand) << L"\r\n"
           << L"AI  : \"" << utf8_to_wide(snap.ml_live_prediction) << L"\"\r\n"
           << L"GOD : " << utf8_to_wide(snap.cpu.god_words) << L"\r\n"
           << L"RIP : 0x" << std::hex << std::setw(4) << std::setfill(L'0') << snap.cpu.rip
           << L"  OP: " << utf8_to_wide(snap.cpu.current_op) << L"\r\n"
           << L"CYC : VM=" << std::dec << snap.cpu.cycles
           << L" | HC=" << snap.cpu.holyc_cycles
           << L" | OISC=" << snap.cpu.subleq_cycles << L"\r\n"
           << L"NET : " << snap.cpu.packets_sent << L" TCP"
           << L" | PoW=" << snap.cpu.pow_hashes
           << L" | A*=" << snap.artifacts.astar_nodes_expanded << L"\r\n"
           << L"[VGA 0xB8000] > \"" << utf8_to_wide(snap.final_vga_output) << L"_\"";

    RECT cpu_text_rc{right_x + 10, right_y + 8, right_x + right_panel_w - 10, right_y + right_h - 8};
    SetTextColor(hdc, RGB(255, 255, 0));
    DrawTextW(hdc, cpu_ws.str().c_str(), -1, &cpu_text_rc, DT_LEFT | DT_WORDBREAK);

    // Bottom Canvas
    RECT canv_rc{12, canvas_y, w - 12, canvas_y + canvas_h};
    int cw = canv_rc.right - canv_rc.left;

    if (c.fake_bsod_mode) {
        HBRUSH blue_dead = CreateSolidBrush(RGB(0, 0, 175));
        FillRect(hdc, &canv_rc, blue_dead);
        DeleteObject(blue_dead);
        DrawEdge(hdc, &canv_rc, EDGE_SUNKEN, BF_RECT);

        SelectObject(hdc, c.font_mono);
        SetTextColor(hdc, RGB(255, 255, 255));
        RECT bs_rc{canv_rc.left + 20, canv_rc.top + 16, canv_rc.right - 20, canv_rc.bottom - 16};
        DrawTextW(
            hdc,
            L"A fatal exception 0xDEADBEEF has occurred at 0xB8000 in VXD HOLYC_AOT(01).\r\n\r\n"
            L"*** STOP: 0x0000DEAD (HELLO_WORLD_EXCEEDED_SANE_ENGINEERING)\r\n"
            L"Причина: Связка HolyC AOT + A* + Рейтрейсинг + SUBLEQ пробила потолок здравого смысла.\r\n\r\n"
            L"Нажмите [ТЕСТ BSOD] еще раз, чтобы вернуться.",
            -1, &bs_rc, DT_LEFT | DT_WORDBREAK
        );
    } else if (c.templeos_vga_mode) {
        HBRUSH tos_white = CreateSolidBrush(RGB(255, 255, 255));
        HBRUSH tos_blue = CreateSolidBrush(RGB(0, 0, 170));
        HBRUSH tos_cyan = CreateSolidBrush(RGB(0, 170, 170));
        FillRect(hdc, &canv_rc, tos_white);

        RECT top_bar{canv_rc.left, canv_rc.top, canv_rc.right, canv_rc.top + 26};
        FillRect(hdc, &top_bar, tos_blue);
        SelectObject(hdc, c.font_mono);
        SetTextColor(hdc, RGB(255, 255, 85));
        DrawTextW(hdc, L" TempleOS V5.03  |  640x480 16-Color VGA  |  HolyC AOT + VM  |  Ring-0 Only", -1, &top_bar, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        RECT inner_box{canv_rc.left + 16, canv_rc.top + 36, canv_rc.right - 16, canv_rc.bottom - 14};
        FillRect(hdc, &inner_box, tos_blue);
        FrameRect(hdc, &inner_box, tos_cyan);

        std::wstring msg = utf8_to_wide(snap.final_vga_output.empty() ? "Hello world" : snap.final_vga_output);
        std::wostringstream tos_txt;
        tos_txt << L" $FG,LTGREEN$// TempleOS DolDoc + AOT Build Output$FG$\r\n"
                << L" I64 HOLY_SHIFT = 2`3;   // Compiled AOT from holyc/hello_temple.HC!\r\n"
                << L" GOD SAYS: " << utf8_to_wide(snap.artifacts.god_oracle_words.empty() ? snap.cpu.god_words : snap.artifacts.god_oracle_words) << L"\r\n"
                << L" HolyC VM Opcodes=" << snap.artifacts.holyc_opcodes_count
                << L" | Cycles=" << snap.artifacts.holyc_vm_cycles << L"\r\n";
        RECT tx_rc{inner_box.left + 10, inner_box.top + 8, inner_box.right - 10, inner_box.top + 96};
        SetTextColor(hdc, RGB(85, 255, 255));
        DrawTextW(hdc, tos_txt.str().c_str(), -1, &tx_rc, DT_LEFT | DT_WORDBREAK);

        SelectObject(hdc, c.font_comic_big);
        SetTextColor(hdc, RGB(255, 255, 85));
        std::wstring cursor_msg = msg + (((c.anim_tick / 10) % 2 == 0) ? L"_" : L" ");
        RECT big_tos{inner_box.left + 10, inner_box.top + 96, inner_box.right - 10, inner_box.bottom - 10};
        DrawTextW(hdc, cursor_msg.c_str(), -1, &big_tos, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        DeleteObject(tos_white);
        DeleteObject(tos_blue);
        DeleteObject(tos_cyan);
    } else {
        c.renderer3d.render_demoscene_frame(c.anim_tick, snap.completed, c.disco_chaos_mode, c.render3d_mode, snap.ml_loss_history);
        c.renderer3d.blit_to_hdc(hdc, canv_rc.left, canv_rc.top, cw, canvas_h);
        DrawEdge(hdc, &canv_rc, EDGE_BUMP, BF_RECT);

        if (!snap.ml_loss_history.empty()) {
            SelectObject(hdc, c.font_ui_ugly);
            SetTextColor(hdc, RGB(0, 255, 0));
            RECT l_lbl{canv_rc.left + 20, canv_rc.bottom - 94, canv_rc.left + 320, canv_rc.bottom - 76};
            DrawTextW(hdc, L"MSE LOSS (NEURAL NET)", -1, &l_lbl, DT_LEFT | DT_SINGLELINE);
        }

        if (!snap.completed) {
            SelectObject(hdc, c.font_comic_title);
            SetTextColor(hdc, RGB(255, 255, 0));
            RECT wait_rc{canv_rc.left + 10, canv_rc.top + 18, canv_rc.right - 10, canv_rc.top + 48};
            DrawTextW(hdc, L"СБОРКА ЯДРА + HOLYC AOT + ИИ + A* + SUBLEQ РАДИ 11 БУКВ...", -1, &wait_rc, DT_CENTER | DT_SINGLELINE);

            SelectObject(hdc, c.font_mono);
            SetTextColor(hdc, RGB(0, 255, 255));
            std::wstring live_out = L"AI=\"" + utf8_to_wide(snap.ml_live_prediction) +
                                    L"\" | VGA=\"" + utf8_to_wide(snap.final_vga_output) + L"\"";
            RECT live_rc{canv_rc.left + 10, canv_rc.top + 52, canv_rc.right - 10, canv_rc.top + 76};
            DrawTextW(hdc, live_out.c_str(), -1, &live_rc, DT_CENTER | DT_SINGLELINE);
        } else {
            draw_clippy_potato(hdc, canv_rc.left + 44, canv_rc.top + 72, c.anim_tick, true, c.font_ui_ugly);

            SelectObject(hdc, c.font_impact);
            SetTextColor(hdc, RGB(255, 255, 255));
            RECT top_lbl{canv_rc.left + 12, canv_rc.top + 8, canv_rc.right - 12, canv_rc.top + 30};
            DrawTextW(hdc, L">>> HOLYC AOT + 24 .O ЯДРА + SGD + ГЕНЕТИКА + RSA + МОРЗЕ + A* + LISP + C99 + BF + SUBLEQ + TCP + 3D <<<", -1, &top_lbl, DT_CENTER | DT_SINGLELINE);

            std::wstring msg = utf8_to_wide(snap.final_vga_output.empty() ? "Hello world" : snap.final_vga_output);
            SelectObject(hdc, c.font_comic_big);
            int letter_step = 40;
            int total_msg_w = static_cast<int>(msg.size()) * letter_step;
            int right_zone_left = canv_rc.left + 320;
            int right_zone_w = std::max(total_msg_w + 40, static_cast<int>(canv_rc.right - right_zone_left - 16));
            int drift_range = std::max(10, (right_zone_w - total_msg_w) / 2);
            int drift_x = static_cast<int>(std::sin(c.anim_tick * (c.disco_chaos_mode ? 0.14 : 0.06)) * drift_range);
            int base_x = right_zone_left + (right_zone_w - total_msg_w) / 2 + drift_x;
            int base_y = canv_rc.top + 64;

            for (size_t idx = 0; idx < msg.size(); ++idx) {
                wchar_t ch_buf[2] = {msg[idx], L'\0'};
                int lx = base_x + static_cast<int>(idx) * letter_step;
                int ly = base_y + static_cast<int>(std::sin(c.anim_tick * (c.disco_chaos_mode ? 0.30 : 0.16) + idx * 0.7) * 16.0);

                SetTextColor(hdc, RGB(0, 0, 0));
                TextOutW(hdc, lx + 5, ly + 5, ch_buf, 1);
                SetTextColor(hdc, RGB(255, 0, 255));
                TextOutW(hdc, lx + 2, ly + 2, ch_buf, 1);
                SetTextColor(hdc, hsv_neon(static_cast<int>(c.anim_tick * 11 + idx * 32)));
                TextOutW(hdc, lx, ly, ch_buf, 1);
            }

            SelectObject(hdc, c.font_comic_small);
            std::wostringstream stats;
            stats << L"HolyC=" << snap.artifacts.holyc_vm_cycles << L"т"
                  << L" | A*=" << snap.artifacts.astar_nodes_expanded
                  << L" | ИИ=" << snap.artifacts.ml_epochs
                  << L" | SUBLEQ=" << snap.artifacts.subleq_cycles
                  << L" | BF=" << snap.artifacts.bf_length
                  << L" | PoW=" << snap.cpu.pow_hashes
                  << L" | " << snap.elapsed_ms << L" мс";
            RECT stat_rc{canv_rc.left + 320, canv_rc.bottom - 48, canv_rc.right - 12, canv_rc.bottom - 26};
            SetTextColor(hdc, RGB(255, 255, 255));
            DrawTextW(hdc, stats.str().c_str(), -1, &stat_rc, DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        }
    }

    SelectObject(hdc, old_f);
    BitBlt(screen_hdc, 0, 0, w, h, hdc, 0, 0, SRCCOPY);
    SelectObject(hdc, old_bmp);
    DeleteObject(bmp);
    DeleteDC(hdc);
    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto& c = ctx();
    switch (msg) {
        case WM_CREATE: {
            c.hwnd = hwnd;

            c.font_comic_big = CreateFontW(50, 0, 0, 0, FW_HEAVY, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Comic Sans MS");
            c.font_comic_title = CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Comic Sans MS");
            c.font_comic_small = CreateFontW(15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Comic Sans MS");
            c.font_impact = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Impact");
            c.font_mono = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier New");
            c.font_ui_ugly = CreateFontW(11, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"MS Sans Serif");

            c.brush_teal_bg = CreateSolidBrush(RGB(0, 128, 128));
            c.brush_term_bg = CreateSolidBrush(RGB(8, 18, 8));

            const struct { int id; const wchar_t* title; } tabs[] = {
                {IDC_TAB_LOG,         L"ЛОГ ЯДРА"},
                {IDC_TAB_HOLYC_AOT,   L"HOLYC AOT"},
                {IDC_TAB_HOLYC_SRC,   L"HOLYC .HC"},
                {IDC_TAB_HOLYC_VM,    L"HOLYC VM"},
                {IDC_TAB_ASTAR,       L"A* МАРШРУТ"},
                {IDC_TAB_ML_GA,       L"ИИ + ГЕН"},
                {IDC_TAB_RSA_MORSE,   L"RSA+МОРЗЕ"},
                {IDC_TAB_XMLSQL,      L"XML / SQL"},
                {IDC_TAB_SCRIPT,      L"GOVNOSCR"},
                {IDC_TAB_TOKENS,      L"ТОКЕНЫ"},
                {IDC_TAB_AST,         L"AST"},
                {IDC_TAB_IR,          L"IR КОД"},
                {IDC_TAB_LISP_C,      L"LISP+C99"},
                {IDC_TAB_BRAINFUCK,   L"BRAINFUCK"},
                {IDC_TAB_SUBLEQ,      L"SUBLEQ"},
                {IDC_TAB_ASM_NET_POW, L"ASM/TCP"},
            };
            for (const auto& t : tabs) {
                HWND b = CreateWindowW(L"BUTTON", t.title, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 64, 22, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(t.id)), GetModuleHandleW(nullptr), nullptr);
                SendMessageW(b, WM_SETFONT, reinterpret_cast<WPARAM>(c.font_ui_ugly), TRUE);
                c.tab_buttons.push_back(b);
            }

            c.edit_log = CreateWindowExW(
                WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY,
                12, 224, 620, 260, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_EDIT_LOG)), GetModuleHandleW(nullptr), nullptr
            );
            SendMessageW(c.edit_log, WM_SETFONT, reinterpret_cast<WPARAM>(c.font_mono), TRUE);
            SendMessageW(c.edit_log, EM_SETLIMITTEXT, 0, 0);

            const struct { int id; const wchar_t* title; } actions[] = {
                {IDC_BTN_REBUILD,   L"ПЕРЕСОБРАТЬ"},
                {IDC_BTN_TURBO,     L"ТУРБО"},
                {IDC_BTN_HOLYC_IDE, L"HOLYC IDE"},
                {IDC_BTN_MUSIC,     L"8-BIT/МОДЕМ"},
                {IDC_BTN_RAYTRACE,  L"РЕЙТРЕЙСИНГ"},
                {IDC_BTN_TEMPLEOS,  L"TEMPLEOS VGA"},
                {IDC_BTN_SWARM,     L"РОЙ ОКОН"},
                {IDC_BTN_DISCO,     L"ДИСКО-ХАОС"},
                {IDC_BTN_BSOD,      L"ТЕСТ BSOD"},
                {IDC_BTN_EXIT,      L"ВЫХОД"},
            };
            for (const auto& a : actions) {
                HWND b = CreateWindowW(L"BUTTON", a.title, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 90, 34, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(a.id)), GetModuleHandleW(nullptr), nullptr);
                SendMessageW(b, WM_SETFONT, reinterpret_cast<WPARAM>(c.font_ui_ugly), TRUE);
                c.action_buttons.push_back(b);
            }

            update_layout(hwnd);
            SetTimer(hwnd, TIMER_ID_ANIM, 40, nullptr);
            start_pipeline_thread(false);
            return 0;
        }

        case WM_SIZE:
            update_layout(hwnd);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;

        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT: {
            HDC hdc_ctrl = reinterpret_cast<HDC>(wParam);
            HWND ctrl = reinterpret_cast<HWND>(lParam);
            if (ctrl == c.edit_log) {
                SetTextColor(hdc_ctrl, RGB(40, 255, 80));
                SetBkColor(hdc_ctrl, RGB(8, 18, 8));
                return reinterpret_cast<LRESULT>(c.brush_term_bg);
            }
            break;
        }

        case WM_TIMER:
            if (wParam == TIMER_ID_ANIM) {
                ++c.anim_tick;
                update_satellite_orbits(hwnd);
                if (c.disco_chaos_mode && (c.anim_tick % 10 == 0)) {
                    const wchar_t* titles[] = {
                        L"ГОСПОДИ ЧТО ПРОИСХОДИТ — Hello world",
                        L"HOLYC AOT + РЕЙТРЕЙСИНГ + A* РАДИ 11 БУКВ!",
                        L"TEMPLEOS RING-0 APPROVED!",
                    };
                    SetWindowTextW(hwnd, titles[(c.anim_tick / 10) % 3]);
                }
                sync_terminal_text();
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id >= IDC_TAB_LOG && id <= IDC_TAB_ASM_NET_POW) {
                set_holyc_ide_mode(false);
            }
            switch (id) {
                case IDC_TAB_LOG:         c.active_tab = ActiveTab::LOG; c.tab_dirty = true; break;
                case IDC_TAB_HOLYC_AOT:   c.active_tab = ActiveTab::HOLYC_AOT; c.tab_dirty = true; break;
                case IDC_TAB_HOLYC_SRC:   c.active_tab = ActiveTab::HOLYC_SRC; c.tab_dirty = true; break;
                case IDC_TAB_HOLYC_VM:    c.active_tab = ActiveTab::HOLYC_VM; c.tab_dirty = true; break;
                case IDC_TAB_ASTAR:       c.active_tab = ActiveTab::ASTAR; c.tab_dirty = true; break;
                case IDC_TAB_ML_GA:       c.active_tab = ActiveTab::ML_GA; c.tab_dirty = true; break;
                case IDC_TAB_RSA_MORSE:   c.active_tab = ActiveTab::RSA_MORSE; c.tab_dirty = true; break;
                case IDC_TAB_XMLSQL:      c.active_tab = ActiveTab::XMLSQL; c.tab_dirty = true; break;
                case IDC_TAB_SCRIPT:      c.active_tab = ActiveTab::SCRIPT; c.tab_dirty = true; break;
                case IDC_TAB_TOKENS:      c.active_tab = ActiveTab::TOKENS; c.tab_dirty = true; break;
                case IDC_TAB_AST:         c.active_tab = ActiveTab::AST; c.tab_dirty = true; break;
                case IDC_TAB_IR:          c.active_tab = ActiveTab::IR; c.tab_dirty = true; break;
                case IDC_TAB_LISP_C:      c.active_tab = ActiveTab::LISP_C; c.tab_dirty = true; break;
                case IDC_TAB_BRAINFUCK:   c.active_tab = ActiveTab::BRAINFUCK; c.tab_dirty = true; break;
                case IDC_TAB_SUBLEQ:      c.active_tab = ActiveTab::SUBLEQ; c.tab_dirty = true; break;
                case IDC_TAB_ASM_NET_POW: c.active_tab = ActiveTab::ASM_NET_POW; c.tab_dirty = true; break;
                case IDC_BTN_REBUILD:     start_pipeline_thread(false); break;
                case IDC_BTN_TURBO:       start_pipeline_thread(true); break;
                case IDC_BTN_HOLYC_IDE: {
                    if (!c.holyc_ide_editing) {
                        set_holyc_ide_mode(true);
                    } else {
                        int len = GetWindowTextLengthW(c.edit_log);
                        std::wstring buf(static_cast<size_t>(len + 1), L'\0');
                        GetWindowTextW(c.edit_log, buf.data(), len + 1);
                        buf.resize(static_cast<size_t>(len));
                        auto res = govno::holyc::execute_custom_holyc_source(wide_to_utf8(buf));
                        std::wostringstream msg_ss;
                        if (!res.success) {
                            msg_ss << L"Ошибка компиляции HolyC:\n" << utf8_to_wide(res.error_msg);
                            MessageBoxW(hwnd, msg_ss.str().c_str(), L"HolyC Live IDE", MB_OK | MB_ICONERROR);
                        } else {
                            msg_ss << L"HolyC успешно скомпилирован (" << res.opcodes_count << L" опкодов, "
                                   << res.vm_cycles << L" тактов)!\n\n"
                                   << L"Печать: " << utf8_to_wide(res.printed_text) << L"\n"
                                   << L"asm { INT 0x80 }: \"" << utf8_to_wide(res.emitted_output) << L"\"\n"
                                   << L"GodTalk: " << utf8_to_wide(res.god_oracle_words);
                            MessageBoxW(hwnd, msg_ss.str().c_str(), L"HolyC Live IDE Выполнено!", MB_OK | MB_ICONINFORMATION);
                        }
                    }
                    break;
                }
                case IDC_BTN_MUSIC:
                    if (c.synth.is_playing()) {
                        c.synth.stop();
                        SetWindowTextW(c.action_buttons[3], L"8-BIT/МОДЕМ");
                    } else {
                        if (c.synth.start()) {
                            SetWindowTextW(c.action_buttons[3], L"СТОП ЗВУК");
                        } else {
                            MessageBoxW(hwnd, L"Аудиокарта не найдена, но 56k модем поёт в сердце ядра!", L"GovnoAudio", MB_OK | MB_ICONINFORMATION);
                        }
                    }
                    break;
                case IDC_BTN_RAYTRACE:
                    c.render3d_mode = (c.render3d_mode == govno::madness::Render3DMode::SPINNING_CUBE)
                        ? govno::madness::Render3DMode::RAYTRACED_SPHERE
                        : govno::madness::Render3DMode::SPINNING_CUBE;
                    SetWindowTextW(
                        c.action_buttons[4],
                        (c.render3d_mode == govno::madness::Render3DMode::RAYTRACED_SPHERE) ? L"3D КУБ" : L"РЕЙТРЕЙСИНГ"
                    );
                    break;
                case IDC_BTN_TEMPLEOS:
                    c.templeos_vga_mode = !c.templeos_vga_mode;
                    c.fake_bsod_mode = false;
                    break;
                case IDC_BTN_SWARM:
                    if (c.swarm_active) destroy_satellite_windows();
                    else spawn_satellite_windows(hwnd);
                    break;
                case IDC_BTN_DISCO:
                    c.disco_chaos_mode = !c.disco_chaos_mode;
                    if (!c.disco_chaos_mode) {
                        SetWindowTextW(hwnd, L"GovnoOS Kernel Builder v5.0 [HOLYC AOT + RAYTRACER] — Hello world");
                    }
                    break;
                case IDC_BTN_BSOD:
                    c.fake_bsod_mode = !c.fake_bsod_mode;
                    break;
                case IDC_BTN_EXIT:
                    ++c.exit_troll_clicks;
                    if (c.exit_troll_clicks == 1) {
                        SetWindowTextW(c.action_buttons.back(), L"НУ НЕ НАДО");
                        MessageBoxW(hwnd, L"Вы серьезно хотите закрыть HolyC AOT, рейтрейсинг, нейросеть, RSA, A* и SUBLEQ?!\nНажмите ещё раз для выхода.", L"Пожалейте ядро", MB_OK | MB_ICONWARNING);
                    } else {
                        DestroyWindow(hwnd);
                    }
                    break;
                default: break;
            }
            sync_terminal_text();
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
            paint_window_double_buffered(hwnd);
            return 0;

        case WM_DESTROY:
            KillTimer(hwnd, TIMER_ID_ANIM);
            destroy_satellite_windows();
            c.synth.stop();
            c.cancel_worker.store(true);
            if (c.worker.joinable()) c.worker.join();
            if (c.font_comic_big) DeleteObject(c.font_comic_big);
            if (c.font_comic_title) DeleteObject(c.font_comic_title);
            if (c.font_comic_small) DeleteObject(c.font_comic_small);
            if (c.font_mono) DeleteObject(c.font_mono);
            if (c.font_ui_ugly) DeleteObject(c.font_ui_ugly);
            if (c.font_impact) DeleteObject(c.font_impact);
            if (c.brush_teal_bg) DeleteObject(c.brush_teal_bg);
            if (c.brush_term_bg) DeleteObject(c.brush_term_bg);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace

int run_cursed_gui_app(HINSTANCE hInstance, int nCmdShow) {
    INITCOMMONCONTROLSEX icc{sizeof(icc), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&icc);

    const wchar_t* kClassName = L"GovnoKernelHelloWorldWndClassV5";
    HICON custom_ico = LoadIconW(hInstance, MAKEINTRESOURCEW(1));

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = hInstance;
    wc.hIcon = custom_ico ? custom_ico : LoadIconW(nullptr, IDI_WARNING);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = kClassName;
    wc.hIconSm = custom_ico ? custom_ico : LoadIconW(nullptr, IDI_ERROR);

    RegisterClassExW(&wc);

    int screen_w = GetSystemMetrics(SM_CXSCREEN);
    int screen_h = GetSystemMetrics(SM_CYSCREEN);
    int win_w = 1200;
    int win_h = 860;
    int pos_x = std::max(0, (screen_w - win_w) / 2);
    int pos_y = std::max(0, (screen_h - win_h) / 2);

    HWND hwnd = CreateWindowExW(
        0,
        kClassName,
        L"GovnoOS Kernel Builder v5.0 [HOLYC AOT + RAYTRACER] — Hello world",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        pos_x, pos_y, win_w, win_h,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hwnd) return 1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}

} // namespace govno::gui
