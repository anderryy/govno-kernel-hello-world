$FG,LTGREEN$// ============================================================================
//  07_holy_compiler_sql_sort.HC — Sorting, Binary Search, SQL & RLE in HolyC
//  Implements in-place sort, binary search, SQL scan & RLE codec in HolyC.
// ============================================================================$FG$

Class CHolySqlRow {
  I64 id;
  I64 ascii_val;
  I64 weight;
  I64 active;
};

#exe {
  StreamPrint("I64 HOLY_SORT_CAP = 16;\n");
  StreamPrint("I64 HOLY_SQL_ROWS = 11;\n");
}

U0 HolyInsertionQuickHybrid(I64 *arr, I64 len) {
  I64 i = 1;
  while (i < len) {
    I64 key = arr[i];
    I64 j = i - 1;
    while (j >= 0 && arr[j] > key) {
      arr[j + 1] = arr[j];
      j = j - 1;
    }
    arr[j + 1] = key;
    i = i + 1;
  }
}

I64 HolyBinarySearch(I64 *arr, I64 len, I64 target) {
  I64 lo = 0;
  I64 hi = len - 1;
  while (lo <= hi) {
    I64 mid = lo + (hi - lo) / 2;
    if (arr[mid] == target) {
      return mid;
    }
    if (arr[mid] < target) {
      lo = mid + 1;
    } else {
      hi = mid - 1;
    }
  }
  return -1;
}

I64 HolyRunSqlScan() {
  CHolySqlRow rows[11];
  I64 target_ascii[11];
  target_ascii[0] = 72;  target_ascii[1] = 101; target_ascii[2] = 108; target_ascii[3] = 108;
  target_ascii[4] = 111; target_ascii[5] = 32;  target_ascii[6] = 119; target_ascii[7] = 111;
  target_ascii[8] = 114; target_ascii[9] = 108; target_ascii[10] = 100;

  I64 i = 0;
  while (i < HOLY_SQL_ROWS) {
    rows[i].id = i + 1;
    rows[i].ascii_val = target_ascii[i];
    rows[i].weight = (i + 1) * 3;
    rows[i].active = 1;
    i = i + 1;
  }

  // SELECT SUM(ascii_val) FROM rows WHERE active = 1 AND 32 <= ascii_val <= 126
  I64 sum = 0;
  i = 0;
  while (i < HOLY_SQL_ROWS) {
    if (rows[i].active == 1) {
      if (32 <= rows[i].ascii_val <= 126) {
        sum = sum + rows[i].ascii_val;
      }
    }
    i = i + 1;
  }
  return sum;
}

I64 HolyRleCompressVerify() {
  I64 raw[16];
  raw[0] = 72; raw[1] = 72; raw[2] = 72;
  raw[3] = 101;
  raw[4] = 108; raw[5] = 108; raw[6] = 108; raw[7] = 108;
  raw[8] = 111;
  raw[9] = 32; raw[10] = 32;
  raw[11] = 119; raw[12] = 111; raw[13] = 114; raw[14] = 108; raw[15] = 100;

  I64 runs[32];
  I64 rlen = 0;
  I64 i = 0;
  while (i < 16) {
    I64 val = raw[i];
    I64 cnt = 1;
    while ((i + cnt) < 16 && raw[i + cnt] == val) {
      cnt = cnt + 1;
    }
    runs[rlen] = cnt;
    runs[rlen + 1] = val;
    rlen = rlen + 2;
    i = i + cnt;
  }

  // Decompress and verify equality
  I64 dec[16];
  I64 dpos = 0;
  I64 k = 0;
  while (k < rlen) {
    I64 c = runs[k];
    I64 v = runs[k + 1];
    I64 t = 0;
    while (t < c && dpos < 16) {
      dec[dpos] = v;
      dpos = dpos + 1;
      t = t + 1;
    }
    k = k + 2;
  }

  I64 matches = 0;
  i = 0;
  while (i < 16) {
    if (dec[i] == raw[i]) {
      matches = matches + 1;
    }
    i = i + 1;
  }
  return matches + rlen;
}

I64 HolyCompilerSqlSortVerify() {
  "$FG,YELLOW$[HolyC SQL/Sort] Running HolyC Sort, BinarySearch, SQL & RLE Codec...$FG$\n";
  I64 arr[11];
  arr[0] = 100; arr[1] = 108; arr[2] = 114; arr[3] = 111; arr[4] = 119;
  arr[5] = 32;  arr[6] = 111; arr[7] = 108; arr[8] = 108; arr[9] = 101; arr[10] = 72;

  HolyInsertionQuickHybrid(arr, 11);
  I64 found_idx = HolyBinarySearch(arr, 11, 72);
  I64 sql_sum = HolyRunSqlScan();
  I64 rle_ok = HolyRleCompressVerify();
  return found_idx + sql_sum + rle_ok;
}
