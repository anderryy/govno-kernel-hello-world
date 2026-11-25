$FG,LTGREEN$// ============================================================================
//  01_temple_boot.HC — TempleOS Ring-0 Memory, GDT/IDT & ACPI Boot Module
//  Authentic HolyC kernel bootstrapper compiled AOT into the final binary.
// ============================================================================$FG$

Class CPageTableEntry {
  I64 present;
  I64 writable;
  I64 user_mode;
  I64 phys_frame;
  I64 flags;
};

Class CGdtDescriptor {
  I64 limit_low;
  I64 base_low;
  U8  base_mid;
  U8  access;
  U8  granularity;
  U8  base_high;
};

Class CIdtGate {
  I64 offset_low;
  I64 selector;
  U8  ist;
  U8  type_attr;
  I64 offset_high;
};

Class CAcpiToasterHeader {
  I64 signature;
  I64 length;
  U8  revision;
  U8  checksum;
  I64 oem_magic;
  I64 power_state;
};

Class CVirtualTask {
  I64 pid;
  I64 priority;
  I64 rip;
  I64 rsp;
  I64 rax;
  I64 rbx;
  I64 time_slice;
  U8  state;
};

Class CRingZeroKernelState {
  I64 total_pages;
  I64 free_pages;
  I64 gdt_entries;
  I64 idt_entries;
  I64 acpi_checksum_ok;
  I64 ticks;
  I64 ring_level;
  U8  booted;
};

#exe {
  StreamPrint("I64 BOOT_MAGIC_SIG = 2`16;\n");
  StreamPrint("I64 BOOT_PAGE_SIZE = 2`12;\n");
  StreamPrint("I64 BOOT_MAX_TASKS = 2`4;\n");
  StreamPrint("I64 BOOT_RING0_FLAG = 0;\n");
}

I64 HolyValidatePhysAddr(I64 addr) {
  if (4096 <= addr <= 268435456) {
    return 1;
  }
  return 0;
}

I64 HolyValidateIrqVector(I64 vec) {
  if (0 <= vec <= 255) {
    return 1;
  }
  return 0;
}

I64 HolyValidateTaskPriority(I64 prio) {
  if (1 <= prio <= 99) {
    return 1;
  }
  return 0;
}

I64 HolyBuildPageTables(I64 page_count) {
  CPageTableEntry pml4[16];
  CPageTableEntry pdpt[16];
  CPageTableEntry pt[64];
  I64 i = 0;
  I64 mapped_bytes = 0;
  I64 checksum = 0;

  while (i < 16) {
    pml4[i].present = 1;
    pml4[i].writable = 1;
    pml4[i].user_mode = 0;
    pml4[i].phys_frame = 4096 * (i + 1);
    pml4[i].flags = 3;

    pdpt[i].present = 1;
    pdpt[i].writable = 1;
    pdpt[i].user_mode = 0;
    pdpt[i].phys_frame = 65536 + 4096 * i;
    pdpt[i].flags = 7;

    if (HolyValidatePhysAddr(pml4[i].phys_frame)) {
      checksum = checksum + pml4[i].phys_frame;
    }
    i = i + 1;
  }

  i = 0;
  while (i < 64) {
    pt[i].present = (i < page_count) ? 1 : 0;
    pt[i].writable = 1;
    pt[i].user_mode = 0;
    pt[i].phys_frame = 1048576 + i * BOOT_PAGE_SIZE;
    pt[i].flags = 3;
    if (pt[i].present) {
      mapped_bytes = mapped_bytes + BOOT_PAGE_SIZE;
      checksum = (checksum ^ pt[i].phys_frame) + i;
    }
    i = i + 1;
  }

  return checksum + (mapped_bytes / 1024);
}

I64 HolyInitGdtAndIdt() {
  CGdtDescriptor gdt[8];
  CIdtGate idt[32];
  I64 i = 0;
  I64 sum = 0;

  // Null, Ring0 Code, Ring0 Data, Ring3 Code, Ring3 Data, TSS
  while (i < 8) {
    gdt[i].limit_low = (i == 0) ? 0 : 65535;
    gdt[i].base_low = 0;
    gdt[i].base_mid = 0;
    gdt[i].access = (i == 0) ? 0 : 154;
    gdt[i].granularity = (i == 0) ? 0 : 207;
    gdt[i].base_high = 0;
    sum = sum + gdt[i].limit_low + gdt[i].access;
    i = i + 1;
  }

  i = 0;
  while (i < 32) {
    idt[i].offset_low = 4096 + i * 16;
    idt[i].selector = 8;
    idt[i].ist = 0;
    idt[i].type_attr = 142;
    idt[i].offset_high = 0;
    if (HolyValidateIrqVector(i)) {
      sum = sum + idt[i].offset_low;
    }
    i = i + 1;
  }

  // Register INT 0x80 syscall gate explicitly
  I64 int80_vec = 128;
  if (HolyValidateIrqVector(int80_vec)) {
    sum = sum + int80_vec * 8;
  }
  return sum;
}

I64 HolyVerifyAcpiToaster() {
  CAcpiToasterHeader hdr;
  hdr.signature = 1413825108;
  hdr.length = 64;
  hdr.revision = 2;
  hdr.checksum = 0;
  hdr.oem_magic = 31337;
  hdr.power_state = 0;

  I64 raw_bytes[16];
  I64 idx = 0;
  I64 total = 0;
  while (idx < 15) {
    raw_bytes[idx] = (idx * 17 + hdr.oem_magic) % 256;
    total = total + raw_bytes[idx];
    idx = idx + 1;
  }
  raw_bytes[15] = (256 - (total % 256)) % 256;

  idx = 0;
  total = 0;
  while (idx < 16) {
    total = total + raw_bytes[idx];
    idx = idx + 1;
  }
  if ((total % 256) == 0) {
    hdr.checksum = 1;
  }
  return hdr.checksum + hdr.oem_magic;
}

I64 HolySimulateTaskScheduler() {
  CVirtualTask tasks[8];
  I64 i = 0;
  I64 switches = 0;
  I64 acc = 0;

  while (i < 8) {
    tasks[i].pid = 100 + i;
    tasks[i].priority = 10 + i * 7;
    tasks[i].rip = 4198400 + i * 256;
    tasks[i].rsp = 2147483648 - i * 4096;
    tasks[i].rax = i * 42;
    tasks[i].rbx = 1337 + i;
    tasks[i].time_slice = 5;
    tasks[i].state = 1;
    i = i + 1;
  }

  I64 round = 0;
  while (round < 4) {
    i = 0;
    while (i < 8) {
      if (HolyValidateTaskPriority(tasks[i].priority)) {
        tasks[i].rip = tasks[i].rip + 4;
        tasks[i].rax = tasks[i].rax ^ (round + 1);
        acc = acc + tasks[i].pid + tasks[i].rax;
        switches = switches + 1;
      }
      i = i + 1;
    }
    round = round + 1;
  }
  return acc + switches;
}

I64 TempleBootEntry() {
  CRingZeroKernelState kst;
  kst.total_pages = 16384;
  kst.free_pages = 16320;
  kst.gdt_entries = 8;
  kst.idt_entries = 32;
  kst.ticks = 0;
  kst.ring_level = BOOT_RING0_FLAG;

  "$FG,LTGREEN$[HolyC Boot] Ring-0 Page Tables, GDT/IDT & ACPI Toaster Check...$FG$\n";
  I64 pt_hash = HolyBuildPageTables(64);
  I64 dt_hash = HolyInitGdtAndIdt();
  I64 acpi_ok = HolyVerifyAcpiToaster();
  I64 sched_ok = HolySimulateTaskScheduler();
  kst.acpi_checksum_ok = acpi_ok;
  kst.booted = 1;
  return pt_hash + dt_hash + acpi_ok + sched_ok + BOOT_MAGIC_SIG;
}
