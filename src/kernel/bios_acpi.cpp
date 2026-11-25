#include "kernel/bios_acpi.hpp"
#include "cursed_macros.hpp"

#include <windows.h>
#include <intrin.h>
#include <algorithm>
#include <cstring>
#include <sstream>

namespace govno::kernel {

HardwareProbeInfo probe_real_hardware_for_comedy() {
    HardwareProbeInfo info;

    // Read real CPU brand string via __cpuid
    int regs[4] = {0, 0, 0, 0};
    __cpuid(regs, static_cast<int>(0x80000000u));
    if (static_cast<unsigned int>(regs[0]) >= 0x80000004u) {
        char brand[49]{};
        __cpuid(reinterpret_cast<int*>(brand), static_cast<int>(0x80000002u));
        __cpuid(reinterpret_cast<int*>(brand + 16), static_cast<int>(0x80000003u));
        __cpuid(reinterpret_cast<int*>(brand + 32), static_cast<int>(0x80000004u));
        brand[48] = '\0';
        std::string raw(brand);
        size_t first = raw.find_first_not_of(' ');
        info.cpu_brand = (first == std::string::npos) ? "Unknown x64 Silicon" : raw.substr(first);
    } else {
        info.cpu_brand = "Generic x86_64 Potato";
    }

    MEMORYSTATUSEX mem{};
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem)) {
        info.total_ram_mb = mem.ullTotalPhys / (1024ULL * 1024ULL);
    }
    info.uptime_sec = GetTickCount64() / 1000ULL;
    return info;
}

size_t simulate_kernel_source_tree_build(
    std::atomic<bool>& cancel_flag,
    const std::function<void(int pct, const std::string& log_msg)>& progress_cb,
    const std::function<void(int ms)>& sleep_cb
) {
    struct KernelUnit {
        const char* source;
        const char* obj;
        const char* warn;
    };

    const KernelUnit units[] = {
        {"arch/x86_64/boot/header_crutches.S",   "arch/x86_64/boot/header_crutches.o",   nullptr},
        {"arch/x86_64/kernel/gdt_spaghetti.c",   "arch/x86_64/kernel/gdt_spaghetti.o",   "warning: alignment of 'gdt' is held together by duct tape"},
        {"arch/x86_64/kernel/idt_panic_only.c",  "arch/x86_64/kernel/idt_panic_only.o",  nullptr},
        {"init/main_govno.c",                    "init/main_govno.o",                    "warning: variable 'sanity' set but never used [-Wlife-choices]"},
        {"mm/slab_leak_allocator.c",             "mm/slab_leak_allocator.o",             "warning: kfree() is implemented as no-op macro"},
        {"mm/page_fault_blamer.c",               "mm/page_fault_blamer.o",               nullptr},
        {"kernel/sched/fair_lottery_rng.c",      "kernel/sched/fair_lottery_rng.o",      nullptr},
        {"kernel/locking/spinlock_forever.c",    "kernel/locking/spinlock_forever.o",    "warning: deadlock detected, shipping to production anyway"},
        {"fs/govnovfs/superblock_borscht.c",     "fs/govnovfs/superblock_borscht.o",     nullptr},
        {"fs/proc/meminfo_fake_numbers.c",       "fs/proc/meminfo_fake_numbers.o",       nullptr},
        {"ipc/pigeon_message_queue.c",           "ipc/pigeon_message_queue.o",           nullptr},
        {"crypto/xor_by_13_military_grade.c",    "crypto/xor_by_13_military_grade.o",    "warning: encryption key is hardcoded as '1234'"},
        {"security/selinux_allow_everything.c",  "security/selinux_allow_everything.o",  nullptr},
        {"net/ipv4/tcp_three_way_awkward.c",     "net/ipv4/tcp_three_way_awkward.o",     nullptr},
        {"net/http/enterprise_json_over_udp.c",  "net/http/enterprise_json_over_udp.o",  "warning: RFC compliance score = -42"},
        {"drivers/acpi/toaster_power_s0.c",      "drivers/acpi/toaster_power_s0.o",      nullptr},
        {"drivers/char/dev_urandom_dice.c",      "drivers/char/dev_urandom_dice.o",      "warning: chosen by fair dice roll, guaranteed random"},
        {"drivers/block/floppy_turbo_overclock.c","drivers/block/floppy_turbo_overclock.o", nullptr},
        {"drivers/input/mouse_ball_lint.c",      "drivers/input/mouse_ball_lint.o",      nullptr},
        {"drivers/gpu/vga_comic_sans_accel.c",   "drivers/gpu/vga_comic_sans_accel.o",   nullptr},
        {"drivers/sound/pc_speaker_dubstep.c",   "drivers/sound/pc_speaker_dubstep.o",   nullptr},
        {"drivers/printer/lpt1_quantum_ink.c",   "drivers/printer/lpt1_quantum_ink.o",   nullptr},
        {"lib/govno_string_strlen_o_n2.c",       "lib/govno_string_strlen_o_n2.o",       "warning: O(N^2) strlen optimization disabled for drama"},
        {"certs/self_signed_trust_me_bro.c",     "certs/self_signed_trust_me_bro.o",     nullptr},
    };

    const size_t total = sizeof(units) / sizeof(units[0]);
    for (size_t i = 0; i < total; ++i) {
        if (cancel_flag.load()) return i;
        int pct = static_cast<int>(((i + 1) * 100) / total);

        // Deliberate funny troll at module 12
        if (i == 11) {
            progress_cb(148, "[CC] Компиляция " + std::string(units[i].source) + " -> ОЙ, 148%! Откат git reset --hard...");
            sleep_cb(220);
            progress_cb(48, "[GIT-ROLLBACK] Продолжаем сборку ядра с 48%...");
            sleep_cb(140);
        } else {
            progress_cb(pct, "[CC] " + std::string(units[i].source) + "  -->  " + units[i].obj);
            if (units[i].warn) {
                progress_cb(pct, std::string("     ^ ") + units[i].warn);
            }
            sleep_cb(75);
        }
    }

    progress_cb(95, "[LD]  Линковка vmlinuz-govno-0.69.elf из 24 объектных файлов (+1204 неразрешенных символов проигнорировано)...");
    sleep_cb(160);
    progress_cb(100, "[OBJCOPY] Сжатие образа ядра bzImage-spaghetti.bin... Готово!");
    return total;
}

} // namespace govno::kernel
