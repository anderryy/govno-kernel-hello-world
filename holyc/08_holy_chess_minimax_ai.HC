$FG,LTGREEN$// ============================================================================
//  08_holy_chess_minimax_ai.HC — Grid BFS/A*, Minimax & Perceptron in HolyC!
//  Implements shortest-path grid solver, game tree minimax & integer MLP.
// ============================================================================$FG$

Class CHolyBfsNode {
  I64 y;
  I64 x;
  I64 dist;
};

Class CHolyPerceptron {
  I64 w0;
  I64 w1;
  I64 bias;
  I64 epochs;
};

#exe {
  StreamPrint("I64 HOLY_GRID_SIZE = 8;\n");
  StreamPrint("I64 HOLY_AI_TARGET = 1;\n");
}

I64 HolyRunGridPathfinder() {
  // 8x8 labyrinth: 0 = open, 1 = wall
  I64 grid[64];
  I64 dist[64];
  I64 qy[64];
  I64 qx[64];
  I64 i = 0;

  while (i < 64) {
    grid[i] = 0;
    dist[i] = -1;
    i = i + 1;
  }

  // Place dungeon walls
  grid[1 * 8 + 1] = 1; grid[1 * 8 + 2] = 1; grid[1 * 8 + 3] = 1;
  grid[2 * 8 + 3] = 1; grid[3 * 8 + 3] = 1; grid[3 * 8 + 4] = 1;
  grid[5 * 8 + 2] = 1; grid[5 * 8 + 3] = 1; grid[5 * 8 + 5] = 1;
  grid[6 * 8 + 5] = 1;

  I64 head = 0;
  I64 tail = 0;
  dist[0] = 0;
  qy[tail] = 0;
  qx[tail] = 0;
  tail = tail + 1;

  I64 dy[4];
  I64 dx[4];
  dy[0] = 1; dy[1] = -1; dy[2] = 0; dy[3] = 0;
  dx[0] = 0; dx[1] = 0;  dx[2] = 1; dx[3] = -1;

  while (head < tail) {
    I64 cy = qy[head];
    I64 cx = qx[head];
    head = head + 1;
    I64 cur_d = dist[cy * 8 + cx];

    I64 d = 0;
    while (d < 4) {
      I64 ny = cy + dy[d];
      I64 nx = cx + dx[d];
      if (0 <= ny <= 7) {
        if (0 <= nx <= 7) {
          I64 idx = ny * 8 + nx;
          if (grid[idx] == 0 && dist[idx] == -1) {
            dist[idx] = cur_d + 1;
            qy[tail] = ny;
            qx[tail] = nx;
            tail = tail + 1;
          }
        }
      }
      d = d + 1;
    }
  }

  return dist[7 * 8 + 7];
}

I64 HolyEvalTicTacToe(I64 *board) {
  // Check rows, cols, diagonals for +1 (X) or -1 (O)
  I64 i = 0;
  while (i < 3) {
    I64 r = board[i * 3] + board[i * 3 + 1] + board[i * 3 + 2];
    if (r == 3)  { return 10; }
    if (r == -3) { return -10; }
    I64 c = board[i] + board[i + 3] + board[i + 6];
    if (c == 3)  { return 10; }
    if (c == -3) { return -10; }
    i = i + 1;
  }
  I64 d1 = board[0] + board[4] + board[8];
  I64 d2 = board[2] + board[4] + board[6];
  if (d1 == 3 || d2 == 3)   { return 10; }
  if (d1 == -3 || d2 == -3) { return -10; }
  return 0;
}

I64 HolyMinimaxSearch(I64 *board, I64 depth, I64 is_max) {
  I64 score = HolyEvalTicTacToe(board);
  if (score == 10)  { return score - depth; }
  if (score == -10) { return score + depth; }
  if (depth >= 4)   { return 0; }

  I64 moves_left = 0;
  I64 i = 0;
  while (i < 9) {
    if (board[i] == 0) {
      moves_left = 1;
    }
    i = i + 1;
  }
  if (moves_left == 0) {
    return 0;
  }

  I64 best = is_max ? -1000 : 1000;
  i = 0;
  while (i < 9) {
    if (board[i] == 0) {
      board[i] = is_max ? 1 : -1;
      I64 val = HolyMinimaxSearch(board, depth + 1, is_max ? 0 : 1);
      board[i] = 0;
      if (is_max) {
        if (val > best) { best = val; }
      } else {
        if (val < best) { best = val; }
      }
    }
    i = i + 1;
  }
  return best;
}

I64 HolyTrainIntegerPerceptron() {
  // Learns printable ASCII classifier (32 <= x <= 126) over integer epochs in HolyC
  CHolyPerceptron net;
  net.w0 = -2;
  net.w1 = 3;
  net.bias = -10;
  net.epochs = 25;

  I64 samples[4];
  I64 labels[4];
  samples[0] = 5;   labels[0] = 0;
  samples[1] = 15;  labels[1] = 0;
  samples[2] = 72;  labels[2] = 1;
  samples[3] = 111; labels[3] = 1;

  I64 ep = 0;
  I64 correct = 0;
  while (ep < net.epochs) {
    correct = 0;
    I64 i = 0;
    while (i < 4) {
      I64 x = samples[i];
      I64 score = net.w0 * x + net.w1 * (x / 10) + net.bias;
      I64 pred = (score >= 0) ? 1 : 0;
      I64 err = labels[i] - pred;
      if (err != 0) {
        net.w0 = net.w0 + err * (x / 16 + 1);
        net.w1 = net.w1 + err;
        net.bias = net.bias + err * 4;
      } else {
        correct = correct + 1;
      }
      i = i + 1;
    }
    ep = ep + 1;
  }
  return correct + net.epochs;
}

I64 HolyGameAiAndGridVerify() {
  "$FG,LTGREEN$[HolyC Minimax/BFS] Solving 8x8 Dungeon, Game Tree & Perceptron...$FG$\n";
  I64 board[9];
  board[0] = 1;  board[1] = 0;  board[2] = -1;
  board[3] = 0;  board[4] = 1;  board[5] = 0;
  board[6] = -1; board[7] = 0;  board[8] = 0;

  I64 mm = HolyMinimaxSearch(board, 0, 1);
  I64 sp = HolyRunGridPathfinder();
  I64 pt = HolyTrainIntegerPerceptron();
  return mm + sp + pt;
}
