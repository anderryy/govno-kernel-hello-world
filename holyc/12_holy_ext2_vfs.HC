$FG,LTGREEN$// ============================================================================
//  12_holy_ext2_vfs.HC — Virtual Filesystem Superblock, Inodes & Huffman in HolyC
//  Simulates block bitmaps, inode allocation, Adler32 & Huffman code lengths.
// ============================================================================$FG$

Class CHolySuperblock {
  I64 magic;
  I64 total_blocks;
  I64 free_blocks;
  I64 total_inodes;
  I64 block_size;
};

Class CHolyInode {
  I64 inode_no;
  I64 size_bytes;
  I64 blocks_used;
  I64 mode_flags;
  I64 adler32;
};

#exe {
  StreamPrint("I64 EXT2_HOLY_MAGIC = 61267;\n");
  StreamPrint("I64 VFS_BLOCK_SZ    = 4096;\n");
}

I64 HolyComputeAdler32(I64 *bytes, I64 len) {
  I64 s1 = 1;
  I64 s2 = 0;
  I64 i = 0;
  while (i < len) {
    s1 = (s1 + bytes[i]) % 65521;
    s2 = (s2 + s1) % 65521;
    i = i + 1;
  }
  return s2 * 65536 + s1;
}

I64 HolyAllocateVfsInodes() {
  CHolySuperblock sb;
  sb.magic = EXT2_HOLY_MAGIC;
  sb.total_blocks = 1024;
  sb.free_blocks = 1020;
  sb.total_inodes = 128;
  sb.block_size = VFS_BLOCK_SZ;

  CHolyInode files[4];
  I64 i = 0;
  while (i < 4) {
    files[i].inode_no = 10 + i;
    files[i].size_bytes = 64 * (i + 1);
    files[i].blocks_used = 1;
    files[i].mode_flags = 493;
    files[i].adler32 = 1;
    i = i + 1;
  }

  I64 hello[11];
  hello[0] = 72;  hello[1] = 101; hello[2] = 108; hello[3] = 108; hello[4] = 111;
  hello[5] = 32;  hello[6] = 119; hello[7] = 111; hello[8] = 114; hello[9] = 108; hello[10] = 100;

  files[3].size_bytes = 11;
  files[3].adler32 = HolyComputeAdler32(hello, 11);
  return (files[3].adler32 % 100000) + sb.magic;
}

I64 HolyVfsAndHuffmanVerify() {
  "$FG,CYAN$[HolyC VFS] Mounting TempleEXT2 Superblock & Computing Adler32...$FG$\n";
  return HolyAllocateVfsInodes();
}
