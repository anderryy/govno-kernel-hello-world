#pragma once

#include "kernel_types.hpp"
#include <atomic>
#include <functional>

namespace govno::kernel {

// Boots GovnoOS Kernel v0.69-LTS, initializes virtual paging + drivers + VFS,
// loads /bin/hello_world.gexe and executes it on GovnoCPU Virtual Machine!
void run_entire_kernel_and_vm_pipeline(std::atomic<bool>& cancel_flag, bool turbo_mode);

} // namespace govno::kernel
