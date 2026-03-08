/*
 * ============================================================================
 *  PROCEDURAL CITY MAP GENERATOR
 * ============================================================================
 *  Generates a grid-based top-down map for a tile-based game.
 *
 *  Usage:
 *    ./mapgen <width> <height> <density> <min_house> <max_house> <output.txt>
 *
 *  Parameters:
 *    width       - Map width in tiles  (e.g. 80)
 *    height      - Map height in tiles (e.g. 60)
 *    density     - House density 0.1 (sparse) to 0.9 (packed)
 *    min_house   - Minimum house interior area in tiles (e.g. 9)
 *    max_house   - Maximum house interior area in tiles (e.g. 36)
 *    output.txt  - Output file path
 *
 *  Example:
 *    ./mapgen 80 60 0.4 9 36 city.txt
 *
 *  Tile Legend:
 *    GF = Grass Floor      CP = Cobble Path     SW = Stone Wall
 *    WF = Wood Floor       WT = Wood Table      FT = Fire Torch
 *    RW = River Water      WD = Wood Door       TB = Top Bed
 *    BB = Bottom Bed       CH = Chair            BK = Bookshelf
 *    CR = Crate            CW = City Wall        WL = Well
 *    TR = Tree             FL = Flower           BL = Barrel
 *    LT = Lantern          MK = Market Stall     SN = Sand/Shore
 *    GT = Gate             CB = Chest            WN = Window
 *    HY = Hay              FG = Forge            ST = Stool
 *    FN = Fence
 * ============================================================================
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ===== TILE TYPES ===== */
enum {
  GF,
  CP,
  SW,
  WF,
  WT,
  FT,
  RW,
  WD,
  TB,
  BB,
  CH,
  BK,
  CR,
  CW,
  WL,
  TR,
  FL,
  BL,
  LT,
  MK,
  SN,
  GT,
  CB,
  WN,
  HY,
  FG,
  ST,
  FN,
  SF,
  AL,
  PW,
  DK,
  PL,
  BT,
  BN,
  GA,
  FO,
  TILE_COUNT
};

static const char *TSTR[] = {"GF", "CP", "SW", "WF", "WT", "FT", "RW", "WD",
                             "TB", "BB", "CH", "BK", "CR", "CW", "WL", "TR",
                             "FL", "BL", "LT", "MK", "SN", "GT", "CB", "WN",
                             "HY", "FG", "ST", "FN", "SF", "AL", "PW", "DK",
                             "PL", "BT", "BN", "GA", "FO"};

/* ===== MAP GLOBALS ===== */
static int W, H;
static int *map;
static float density;
static int min_area, max_area;

/* Layout boundaries */
static int cl, cr, ct, cb; /* city wall left/right/top/bottom */
static int gx1, gx2;       /* gate x range */
static int river_y;        /* y where shore starts */

/* ===== UTILITIES ===== */
static int rr(int lo, int hi) {
  if (lo >= hi)
    return lo;
  return lo + rand() % (hi - lo + 1);
}
static int G(int x, int y) {
  if (x < 0 || x >= W || y < 0 || y >= H)
    return -1;
  return map[y * W + x];
}
static void P(int x, int y, int t) {
  if (x >= 0 && x < W && y >= 0 && y < H)
    map[y * W + x] = t;
}
static int is_road(int x, int y) {
  int t = G(x, y);
  return t == CP || t == GT;
}
static int is_free(int x, int y) {
  int t = G(x, y);
  return t == GF || t == FL;
}

/* ===== STEP 1: INITIALIZE ===== */
static void init_map(void) {
  map = calloc(W * H, sizeof(int));
  if (!map) {
    fprintf(stderr, "Out of memory\n");
    exit(1);
  }
  for (int i = 0; i < W * H; i++)
    map[i] = GF;
}

/* ===== STEP 2: RIVER ===== */
static void place_river(void) {
  int depth = (H > 40) ? 4 : 3;
  river_y = H - depth - 1; /* 1 row shore + depth rows water */

  /* Add some waviness to the shoreline */
  int *shore_y = malloc(W * sizeof(int));
  for (int x = 0; x < W; x++)
    shore_y[x] = river_y;

  /* Gentle wave pattern */
  double phase = (double)(rand() % 1000) / 100.0;
  for (int x = 0; x < W; x++) {
    int wave = (int)(sin((double)x / 8.0 + phase) * 1.2);
    shore_y[x] = river_y + wave;
    if (shore_y[x] < river_y - 1)
      shore_y[x] = river_y - 1;
    if (shore_y[x] >= H - depth + 1)
      shore_y[x] = H - depth + 1;
  }

  for (int x = 0; x < W; x++) {
    int sy = shore_y[x];
    P(x, sy, SN);
    for (int y = sy + 1; y < H; y++)
      P(x, y, RW);
  }
  free(shore_y);
}

/* ===== STEP 3: CITY WALLS ===== */
static int dock_rows; /* number of dock rows between wall and shore */

static void place_walls(void) {
  ct = 1;
  cl = 1;
  cr = W - 2;
  dock_rows = (H > 80) ? 3 : 2;
  cb = river_y - dock_rows - 1;
  int gate_w = 3 + (W > 60);
  gx1 = W / 2 - gate_w / 2;
  gx2 = gx1 + gate_w - 1;

  /* === 2-wide walls === */
  for (int x = cl; x <= cr; x++) {
    P(x, ct, CW);
    P(x, ct + 1, CW);
  }
  for (int x = cl; x <= cr; x++) {
    if (x >= gx1 && x <= gx2) {
      P(x, cb, GT);
      P(x, cb - 1, GT);
      for (int y = cb + 1; y <= river_y; y++)
        P(x, y, CP);
    } else {
      P(x, cb, CW);
      P(x, cb - 1, CW);
    }
  }
  for (int y = ct; y <= cb; y++) {
    P(cl, y, CW);
    P(cl + 1, y, CW);
    P(cr, y, CW);
    P(cr - 1, y, CW);
  }
  /* 3x3 corner towers */
  for (int dy = 0; dy < 3; dy++)
    for (int dx = 0; dx < 3; dx++) {
      P(cl + dx, ct + dy, CW);
      P(cr - dx, ct + dy, CW);
      P(cl + dx, cb - dy, CW);
      P(cr - dx, cb - dy, CW);
    }
}

/* ===== STEP 3.5: DOCKS ===== */
static void place_docks(void) {
  int dock_top = cb + 1;
  int dock_bot = river_y - 1; /* row above the shore */

  /* Fill dock area with wooden planks */
  for (int y = dock_top; y <= dock_bot; y++)
    for (int x = cl + 1; x <= cr - 1; x++) {
      int t = G(x, y);
      if (t == GF || t == SN)
        P(x, y, DK);
    }

  /* Build piers extending into the water */
  int pier_spacing = rr(8, 14);
  for (int x = cl + 4; x <= cr - 4; x += pier_spacing + rr(-2, 2)) {
    /* Skip if too close to gate */
    if (x >= gx1 - 1 && x <= gx2 + 1)
      continue;

    /* Pier: 2 tiles wide, extends from dock into water */
    for (int y = dock_top; y <= river_y + 2 && y < H; y++) {
      int t = G(x, y);
      if (t == SN || t == RW || t == GF) {
        P(x, y, DK);
      }
    }
    /* Posts on the sides of the pier */
    for (int y = dock_top; y <= river_y + 2 && y < H; y++) {
      if (G(x - 1, y) == RW || G(x - 1, y) == SN)
        P(x - 1, y, PL);
      if (G(x + 1, y) == RW || G(x + 1, y) == SN)
        P(x + 1, y, PL);
    }
  }

  /* Dock decorations: barrels, crates, rope */
  for (int y = dock_top; y <= dock_bot; y++)
    for (int x = cl + 2; x <= cr - 2; x++) {
      if (G(x, y) != DK)
        continue;
      if (rand() % 18 == 0)
        P(x, y, BL);
      else if (rand() % 20 == 0)
        P(x, y, CR);
    }

  printf("[INFO] Docks placed: %d rows, y=%d to %d\n", dock_bot - dock_top + 1,
         dock_top, dock_bot);
}

/* ===== STEP 3.6: RIVER INLET ===== */
/* Winding canal with docks, boats, merchants */
static void place_inlet(void) {
  int inlet_depth = H / 7;
  if (inlet_depth < 6)  inlet_depth = 6;
  if (inlet_depth > 35) inlet_depth = 35;
  int base_wx1 = gx1, base_wx2 = gx2;
  int inlet_top = cb - inlet_depth;
  if (inlet_top < ct + 6) inlet_top = ct + 6;
  double phase = (double)(rand() % 1000) / 100.0;

  /* Winding water + shore + docks */
  for (int y = cb; y >= inlet_top; y--) {
    int dist = cb - y;
    int off = (int)(sin((double)dist / 4.0 + phase) * 1.8);
    int wx1 = base_wx1 + off, wx2 = base_wx2 + off;
    if (wx1 < cl+4) { wx2 += (cl+4-wx1); wx1 = cl+4; }
    if (wx2 > cr-4) { wx1 -= (wx2-cr+4); wx2 = cr-4; }
    for (int x = wx1; x <= wx2; x++) P(x, y, RW);
    if (wx1-1 > cl+2) P(wx1-1, y, SN);
    if (wx2+1 < cr-2) P(wx2+1, y, SN);
    for (int dx = 2; dx <= 3; dx++) {
      int lx = wx1-dx, rx = wx2+dx;
      if (lx > cl+2 && (G(lx,y)==GF||G(lx,y)==FL||G(lx,y)==CP)) P(lx,y,DK);
      if (rx < cr-2 && (G(rx,y)==GF||G(rx,y)==FL||G(rx,y)==CP)) P(rx,y,DK);
    }
  }
  /* Rounded pool end */
  for (int dx = -2; dx <= (base_wx2-base_wx1)+2; dx++) {
    int x = base_wx1 + dx;
    if (x > cl+3 && x < cr-3 && G(x,inlet_top)!=CW && G(x,inlet_top)!=RW)
      P(x, inlet_top, SN);
  }
  /* Bridges */
  int bsp = rr(6,10);
  for (int y = cb-bsp; y >= inlet_top+2; y -= bsp+rr(-1,2)) {
    int dist=cb-y; int off=(int)(sin((double)dist/4.0+phase)*1.8);
    int wx1=base_wx1+off, wx2=base_wx2+off;
    if (wx1<cl+4) wx1=cl+4; if (wx2>cr-4) wx2=cr-4;
    for (int x = wx1; x <= wx2; x++) P(x, y, CP);
    if (wx1-1>cl+2) P(wx1-1,y,PL);
    if (wx2+1<cr-2) P(wx2+1,y,PL);
  }
  /* Boats on the water */
  for (int y = cb-3; y >= inlet_top+3; y -= rr(5,9)) {
    int dist=cb-y; int off=(int)(sin((double)dist/4.0+phase)*1.8);
    int wx1=base_wx1+off; if (wx1<cl+4) wx1=cl+4;
    if (G(wx1,y)==RW) P(wx1,y,BT);
    if (G(wx1+1,y)==RW) P(wx1+1,y,BT);
  }
  /* Merchant benches & stalls on docks */
  for (int y = cb-2; y >= inlet_top+2; y -= rr(3,5)) {
    int dist=cb-y; int off=(int)(sin((double)dist/4.0+phase)*1.8);
    int wx1=base_wx1+off, wx2=base_wx2+off;
    if (wx1<cl+4) wx1=cl+4; if (wx2>cr-4) wx2=cr-4;
    int lx=wx1-3, rx=wx2+3;
    if (lx>cl+2 && G(lx,y)==DK) P(lx,y,(rand()%2)?BN:MK);
    if (rx<cr-2 && G(rx,y)==DK) P(rx,y,(rand()%2)?BN:MK);
  }
  /* Lanterns */
  for (int y=cb-2; y>=inlet_top+2; y-=rr(3,6)) {
    int dist=cb-y; int off=(int)(sin((double)dist/4.0+phase)*1.8);
    int wx1=base_wx1+off, wx2=base_wx2+off;
    if (wx1<cl+4) wx1=cl+4; if (wx2>cr-4) wx2=cr-4;
    if (wx1-2>cl+2 && G(wx1-2,y)==DK) P(wx1-2,y,LT);
    if (wx2+2<cr-2 && G(wx2+2,y)==DK) P(wx2+2,y,LT);
  }
  /* Barrels / crates */
  for (int y=cb-1; y>=inlet_top+1; y--)
    for (int dx=2;dx<=3;dx++) {
      int dist=cb-y; int off=(int)(sin((double)dist/4.0+phase)*1.8);
      int wx1=base_wx1+off,wx2=base_wx2+off;
      if(wx1<cl+4)wx1=cl+4; if(wx2>cr-4)wx2=cr-4;
      int lx=wx1-dx,rx=wx2+dx;
      if(lx>cl+2&&G(lx,y)==DK&&rand()%14==0) P(lx,y,(rand()%2)?BL:CR);
      if(rx<cr-2&&G(rx,y)==DK&&rand()%14==0) P(rx,y,(rand()%2)?BL:CR);
    }
  printf("[INFO] River inlet: %d tiles deep (winding), y=%d to %d\n",
         inlet_depth, inlet_top, cb);
}

/* ===== STEP 4: ROAD NETWORK (organic) ===== */
static void generate_roads(void) {
  int il = cl + 3, ir = cr - 3;
  int it = ct + 3, ib = cb - 3;
  int mx = W / 2;
  int city_w = ir - il;
  int city_h = ib - it;

  /* ---- 1. Main Avenue: 2-wide from gate to top ---- */
  for (int y = it; y < cb; y++) {
    P(mx, y, CP);
    P(mx + 1, y, CP);
  }

  /* ---- 2. Major Cross Streets (branching from avenue) ---- */
  /* Distribute irregularly, each extends different distances L/R */
  int num_cross = 2 + city_h / 18;
  if (num_cross > 7)
    num_cross = 7;
  int cross_ys[10];
  int n_cross = 0;

  for (int i = 0; i < num_cross; i++) {
    int zone = city_h / (num_cross + 1);
    int y = it + zone * (i + 1) + rr(-zone / 3, zone / 3);
    if (y <= it + 2)
      y = it + 3;
    if (y >= ib - 2)
      y = ib - 3;

    /* Extend different distances left and right (NOT full width) */
    int left_reach = rr(city_w / 5, city_w * 2 / 3);
    int right_reach = rr(city_w / 5, city_w * 2 / 3);
    int x_left = mx - left_reach;
    int x_right = mx + 1 + right_reach;
    if (x_left < il + 1)
      x_left = il + 1;
    if (x_right > ir - 1)
      x_right = ir - 1;

    for (int x = x_left; x <= x_right; x++)
      P(x, y, CP);

    cross_ys[n_cross++] = y;
  }

  /* ---- 3. Secondary Side Streets ---- */
  /* Branch vertical streets from cross streets at random positions */
  for (int c = 0; c < n_cross; c++) {
    int cy = cross_ys[c];
    int n_branches = rr(2, 3 + city_w / 40);

    for (int b = 0; b < n_branches; b++) {
      /* Pick a point on the cross street, avoiding main avenue */
      int bx;
      if (rand() % 2 == 0)
        bx = rr(il + 2, mx - 4);
      else
        bx = rr(mx + 4, ir - 2);

      if (!is_road(bx, cy))
        continue;

      /* Grow up and/or down a random distance */
      int up_len = rr(4, city_h / 3 + 3);
      int down_len = rr(4, city_h / 3 + 3);

      /* Sometimes make one direction short/zero (dead-end feel) */
      if (rand() % 3 == 0)
        up_len = rr(0, 3);
      if (rand() % 4 == 0)
        down_len = rr(0, 3);
      if (up_len <= 1 && down_len <= 1)
        up_len = rr(5, 12);

      for (int dy = 1; dy <= up_len; dy++) {
        int ny = cy - dy;
        if (ny <= it)
          break;
        P(bx, ny, CP);
      }
      for (int dy = 1; dy <= down_len; dy++) {
        int ny = cy + dy;
        if (ny >= ib)
          break;
        P(bx, ny, CP);
      }
    }
  }

  /* ---- 4. Short connecting alleys ---- */
  /* Small horizontal segments that connect nearby streets */
  int num_alleys = rr(3, 5 + city_w / 25);
  for (int a = 0; a < num_alleys; a++) {
    int ay = rr(it + 3, ib - 3);
    int ax = rr(il + 3, ir - 15);
    int alen = rr(4, 12);

    /* Only draw if at least one end is near a road */
    int connected = 0;
    for (int dy = -1; dy <= 1; dy++) {
      if (is_road(ax, ay + dy))
        connected++;
      if (is_road(ax + alen, ay + dy))
        connected++;
    }
    if (connected >= 1) {
      for (int dx = 0; dx < alen && ax + dx <= ir - 1; dx++)
        P(ax + dx, ay, CP);
    }
  }

  /* ---- 5. Waterfront road (partial, near docks) ---- */
  int wf_left = rr(il, mx - 8);
  int wf_right = rr(mx + 5, ir);
  for (int x = wf_left; x <= wf_right; x++)
    P(x, ib, CP);
}

/* ===== STEP 5: PLAZA ===== */
static void generate_plaza(void) {
  int cx = W / 2;
  int cy = (ct + cb) / 2;
  int r = (W > 60 && H > 40) ? 4 : 3;

  for (int dy = -r; dy <= r; dy++)
    for (int dx = -r; dx <= r; dx++)
      if (dx * dx + dy * dy <= r * r + r) {
        int x = cx + dx, y = cy + dy;
        if (x > cl + 1 && x < cr - 1 && y > ct + 1 && y < cb - 1)
          P(x, y, CP);
      }

  P(cx, cy, WL); /* Well in center */

  /* Lanterns at corners of plaza */
  if (G(cx - r + 1, cy - r + 1) == CP)
    P(cx - r + 1, cy - r + 1, LT);
  if (G(cx + r - 1, cy - r + 1) == CP)
    P(cx + r - 1, cy - r + 1, LT);
  if (G(cx - r + 1, cy + r - 1) == CP)
    P(cx - r + 1, cy + r - 1, LT);
  if (G(cx + r - 1, cy + r - 1) == CP)
    P(cx + r - 1, cy + r - 1, LT);

  /* Market stalls near plaza */
  int dirs[][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
  for (int i = 0; i < 4; i++) {
    int sx = cx + dirs[i][0] * (r + 2);
    int sy = cy + dirs[i][1] * (r + 2);
    if (is_free(sx, sy))
      P(sx, sy, MK);
  }
}

/* ===== STEP 6: HOUSES ===== */

/* Check if rectangle edge is adjacent to road somewhere */
static int rect_touches_road(int x1, int y1, int x2, int y2) {
  /* Check all 4 edges' external neighbors */
  for (int x = x1; x <= x2; x++) {
    if (is_road(x, y1 - 1))
      return 1;
    if (is_road(x, y2 + 1))
      return 1;
  }
  for (int y = y1; y <= y2; y++) {
    if (is_road(x1 - 1, y))
      return 1;
    if (is_road(x2 + 1, y))
      return 1;
  }
  return 0;
}

/* Place walls around a rectangular interior, find and place door facing road */
static void place_rect_walls_and_door(int x1, int y1, int x2, int y2) {
  /* Walls on all 4 sides (1 tile outside the interior) */
  int wx1 = x1 - 1, wy1 = y1 - 1, wx2 = x2 + 1, wy2 = y2 + 1;

  for (int x = wx1; x <= wx2; x++) {
    P(x, wy1, SW);
    P(x, wy2, SW);
  }
  for (int y = wy1; y <= wy2; y++) {
    P(wx1, y, SW);
    P(wx2, y, SW);
  }

  /* Add windows on longer walls */
  if (x2 - x1 >= 3) {
    int mid = (x1 + x2) / 2;
    if (G(mid, wy1) == SW)
      P(mid, wy1, WN);
    if (G(mid, wy2) == SW)
      P(mid, wy2, WN);
  }
  if (y2 - y1 >= 3) {
    int mid = (y1 + y2) / 2;
    if (G(wx1, mid) == SW)
      P(wx1, mid, WN);
    if (G(wx2, mid) == SW)
      P(wx2, mid, WN);
  }

  /* Place door: find wall tile adjacent to a road */
  int door_placed = 0;
  /* Prefer bottom wall, then top, then sides */
  int sides[][3] = {
      /* dx_start, dy, horizontal? */
      {0, 1, 1},  /* bottom */
      {0, -1, 1}, /* top */
      {1, 0, 0},  /* right */
      {-1, 0, 0}  /* left */
  };
  for (int s = 0; s < 4 && !door_placed; s++) {
    if (sides[s][2]) { /* horizontal wall */
      int wy = (sides[s][1] == 1) ? wy2 : wy1;
      int check_y = wy + sides[s][1];
      for (int x = x1; x <= x2; x++) {
        if (is_road(x, check_y) && G(x, wy) == SW) {
          P(x, wy, WD);
          door_placed = 1;
          break;
        }
      }
    } else { /* vertical wall */
      int wx = (sides[s][0] == 1) ? wx2 : wx1;
      int check_x = wx + sides[s][0];
      for (int y = y1; y <= y2; y++) {
        if (is_road(check_x, y) && G(wx, y) == SW) {
          P(wx, y, WD);
          door_placed = 1;
          break;
        }
      }
    }
  }
  /* Fallback: place door on any wall tile adjacent to road */
  if (!door_placed) {
    for (int x = wx1; x <= wx2 && !door_placed; x++)
      for (int y = wy1; y <= wy2 && !door_placed; y++)
        if (G(x, y) == SW && (is_road(x - 1, y) || is_road(x + 1, y) ||
                              is_road(x, y - 1) || is_road(x, y + 1))) {
          P(x, y, WD);
          door_placed = 1;
        }
  }
  /* Last resort: any wall tile gets a door */
  if (!door_placed) {
    P((x1 + x2) / 2, wy2, WD);
  }
}

/* Furnish a rectangular house interior */
static void furnish_rect(int x1, int y1, int x2, int y2) {
  int iw = x2 - x1 + 1;
  int ih = y2 - y1 + 1;
  int area_val = iw * ih;

  /* Always place a torch on an interior wall-adjacent tile */
  P(x1, y1, FT);
  if (iw > 2 && ih > 2)
    P(x2, y2, FT);

  /* Bed in a corner (TB on top, BB below) */
  if (ih >= 2) {
    int bx = x2, by = y1;
    P(bx, by, TB);
    P(bx, by + 1, BB);
  }

  /* Table + chair(s) */
  if (iw >= 3 && ih >= 3) {
    int tx = x1 + iw / 2, ty = y1 + ih / 2;
    P(tx, ty, WT);
    if (G(tx - 1, ty) == WF)
      P(tx - 1, ty, CH);
    if (G(tx + 1, ty) == WF)
      P(tx + 1, ty, CH);
  } else if (iw >= 2) {
    P(x1 + 1, y2, WT);
    if (G(x1, y2) == WF)
      P(x1, y2, CH);
  }

  /* Larger houses get more furniture */
  if (area_val >= 16) {
    /* Bookshelf against a wall */
    if (G(x1, y2) == WF)
      P(x1, y2, BK);
    /* Crate/barrel */
    if (G(x1 + 1, y1) == WF)
      P(x1 + 1, y1, CR);
    /* Second bed for bigger houses */
    if (area_val >= 20 && ih >= 3 && G(x2, y2 - 1) == WF && G(x2, y2) == WF) {
      P(x2, y2 - 1, TB);
      P(x2, y2, BB);
    }
  }
  if (area_val >= 25) {
    /* Chest */
    if (G(x2 - 1, y1) == WF)
      P(x2 - 1, y1, CB);
    /* Extra torch */
    if (G(x1, y1 + 1) == WF)
      P(x1, y1 + 1, FT);
    /* Barrel */
    if (G(x1 + 1, y2) == WF)
      P(x1 + 1, y2, BL);
  }
  if (area_val >= 30) {
    /* Stool */
    if (iw >= 4 && G(x1 + 2, y2) == WF)
      P(x1 + 2, y2, ST);
    /* Hay */
    if (G(x2 - 1, y2) == WF)
      P(x2 - 1, y2, HY);
  }
}

/* Try to place one rectangular house at (hx, hy) with given interior dims */
static int try_place_rect_house(int hx, int hy, int iw, int ih) {
  int x1 = hx, y1 = hy, x2 = hx + iw - 1, y2 = hy + ih - 1;

  /* Check the full footprint including walls (1 tile border) is clear */
  for (int y = y1 - 2; y <= y2 + 2; y++)
    for (int x = x1 - 2; x <= x2 + 2; x++) {
      int t = G(x, y);
      if (x >= x1 - 1 && x <= x2 + 1 && y >= y1 - 1 && y <= y2 + 1) {
        /* Wall/interior zone: must be on clear grass (not roads!) */
        if (t != GF && t != FL)
          return 0;
      }
    }

  /* Must be adjacent to a road for door access */
  if (!rect_touches_road(x1 - 1, y1 - 1, x2 + 1, y2 + 1))
    return 0;

  /* Place interior floor */
  for (int y = y1; y <= y2; y++)
    for (int x = x1; x <= x2; x++)
      P(x, y, WF);

  /* Place walls and door */
  place_rect_walls_and_door(x1, y1, x2, y2);

  /* Furnish */
  furnish_rect(x1, y1, x2, y2);

  return 1;
}

/* Try to place an L-shaped house */
static int try_place_l_house(int hx, int hy, int target_a) {
  /* L-shape = two overlapping rectangles */
  /* Main rect + extension attached to one side */
  int mw = rr(3, (int)sqrt((double)target_a) + 2);
  int mh = rr(3, (int)sqrt((double)target_a) + 2);
  if (mw > 8)
    mw = 8;
  if (mh > 8)
    mh = 8;

  /* Extension */
  int ew = rr(2, mw - 1);
  int eh = rr(2, mh / 2 + 1);
  if (eh < 2)
    eh = 2;

  int total = mw * mh + ew * eh;
  if (total < min_area || total > max_area * 1.3)
    return 0;

  /* Choose which corner to attach extension: 0=bottom-right, 1=bottom-left */
  int corner = rand() % 2;

  /* Compute bounding box and check clearance */
  int bx1, by1, bx2, by2;
  /* Main rect */
  int mx1 = hx, my1 = hy, mx2 = hx + mw - 1, my2 = hy + mh - 1;
  /* Extension rect */
  int ex1, ey1, ex2, ey2;
  if (corner == 0) { /* extend right-bottom */
    ex1 = mx2 + 1;
    ey1 = my2 - eh + 1;
    ex2 = mx2 + ew;
    ey2 = my2;
  } else { /* extend left-bottom */
    ex1 = mx1 - ew;
    ey1 = my2 - eh + 1;
    ex2 = mx1 - 1;
    ey2 = my2;
  }

  bx1 = (mx1 < ex1) ? mx1 : ex1;
  by1 = (my1 < ey1) ? my1 : ey1;
  bx2 = (mx2 > ex2) ? mx2 : ex2;
  by2 = (my2 > ey2) ? my2 : ey2;

  /* Check full bounding box + 2 tile buffer is clear */
  for (int y = by1 - 2; y <= by2 + 2; y++)
    for (int x = bx1 - 2; x <= bx2 + 2; x++) {
      int t = G(x, y);
      if (t != GF && t != FL)
        return 0;
    }

  /* Check road adjacency for the combined shape */
  int road_adj = 0;
  /* Check around main rect */
  if (rect_touches_road(mx1 - 1, my1 - 1, mx2 + 1, my2 + 1))
    road_adj = 1;
  if (rect_touches_road(ex1 - 1, ey1 - 1, ex2 + 1, ey2 + 1))
    road_adj = 1;
  if (!road_adj)
    return 0;

  /* Place main rect interior */
  for (int y = my1; y <= my2; y++)
    for (int x = mx1; x <= mx2; x++)
      P(x, y, WF);

  /* Place extension interior */
  for (int y = ey1; y <= ey2; y++)
    for (int x = ex1; x <= ex2; x++)
      P(x, y, WF);

  /* Walls: surround all WF tiles that border non-WF */
  for (int y = by1 - 1; y <= by2 + 1; y++)
    for (int x = bx1 - 1; x <= bx2 + 1; x++) {
      if (G(x, y) == WF)
        continue;
      /* Check if adjacent to any WF tile */
      int adj = 0;
      for (int d = 0; d < 4; d++) {
        int nx = x + (d == 0 ? -1 : d == 1 ? 1 : 0);
        int ny = y + (d == 2 ? -1 : d == 3 ? 1 : 0);
        if (G(nx, ny) == WF) {
          adj = 1;
          break;
        }
      }
      /* Also check diagonals for corners */
      if (!adj) {
        int dd[][2] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
        for (int d = 0; d < 4; d++)
          if (G(x + dd[d][0], y + dd[d][1]) == WF) {
            adj = 1;
            break;
          }
      }
      if (adj && (G(x, y) == GF || G(x, y) == FL))
        P(x, y, SW);
    }

  /* Place door: find SW tile adjacent to road */
  int door_placed = 0;
  for (int y = by1 - 1; y <= by2 + 1 && !door_placed; y++)
    for (int x = bx1 - 1; x <= bx2 + 1 && !door_placed; x++)
      if (G(x, y) == SW && (is_road(x - 1, y) || is_road(x + 1, y) ||
                            is_road(x, y - 1) || is_road(x, y + 1))) {
        P(x, y, WD);
        door_placed = 1;
      }

  /* Windows: find wall tiles on longer runs */
  for (int y = by1 - 1; y <= by2 + 1; y++)
    for (int x = bx1 - 1; x <= bx2 + 1; x++)
      if (G(x, y) == SW && rand() % 6 == 0)
        P(x, y, WN);

  /* Furnish main room */
  furnish_rect(mx1, my1, mx2, my2);

  /* Furnish extension with a few items */
  if (ew >= 2 && eh >= 2) {
    P(ex1, ey1, FT);
    if (G((ex1 + ex2) / 2, (ey1 + ey2) / 2) == WF)
      P((ex1 + ex2) / 2, (ey1 + ey2) / 2, CR);
    if (G(ex2, ey2) == WF)
      P(ex2, ey2, BL);
  }

  return 1;
}

/* Place houses across the city */
static void generate_houses(void) {
  int il = cl + 3, ir = cr - 3;
  int it = ct + 3, ib = cb - 3;
  int city_area = (ir - il) * (ib - it);
  int avg_house = (min_area + max_area) / 2 + 10; /* +10 for walls */
  int target_houses = (int)(city_area * density / (double)avg_house);

  int placed = 0;
  int max_attempts = target_houses * 15;

  for (int att = 0; att < max_attempts && placed < target_houses; att++) {
    int hx = rr(il + 1, ir - 6);
    int hy = rr(it + 1, ib - 6);

    int a = rr(min_area, max_area);
    int do_l = (rand() % 100) < 30; /* 30% chance L-shape */

    if (do_l) {
      if (try_place_l_house(hx, hy, a))
        placed++;
    } else {
      /* Generate rectangular dimensions */
      int min_d = (int)ceil(sqrt((double)min_area));
      if (min_d < 2)
        min_d = 2;
      int max_d = (int)sqrt((double)max_area) + 1;
      if (max_d > 10)
        max_d = 10;

      int iw = rr(min_d, max_d);
      int ih = a / iw;
      if (ih < 2)
        ih = 2;
      if (iw * ih < min_area)
        ih = (min_area + iw - 1) / iw;
      if (iw * ih > max_area) {
        ih = max_area / iw;
        if (ih < 2)
          continue;
      }

      if (try_place_rect_house(hx, hy, iw, ih))
        placed++;
    }
  }

  printf("[INFO] Placed %d houses (target: %d)\n", placed, target_houses);
}

/* ===== STEP 7: OUTDOOR DECORATIONS ===== */
static void add_decorations(void) {
  int il = cl + 2, ir = cr - 2;
  int it = ct + 2, ib = cb - 2;

  /* === Trees along inside of city walls === */
  for (int x = il; x <= ir; x += rr(2, 4)) {
    if (is_free(x, it))
      P(x, it, TR);
    if (is_free(x, ib))
      P(x, ib, TR);
  }
  for (int y = it; y <= ib; y += rr(2, 4)) {
    if (is_free(il, y))
      P(il, y, TR);
    if (is_free(ir, y))
      P(ir, y, TR);
  }

  /* === Street lanterns along roads === */
  for (int y = it; y <= ib; y += rr(4, 7)) {
    for (int x = il; x <= ir; x += rr(5, 8)) {
      /* Place lantern on grass tile adjacent to road */
      if (is_free(x, y) && (is_road(x - 1, y) || is_road(x + 1, y) ||
                            is_road(x, y - 1) || is_road(x, y + 1))) {
        P(x, y, LT);
      }
    }
  }

  /* === Scatter flowers and small trees in open grass areas === */
  for (int y = it; y <= ib; y++) {
    for (int x = il; x <= ir; x++) {
      if (G(x, y) != GF)
        continue;

      /* Check if this is in an open area (not right next to a building) */
      int near_building = 0;
      for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
          int t = G(x + dx, y + dy);
          if (t == SW || t == WD || t == WN)
            near_building = 1;
        }
      if (near_building)
        continue;

      int r_val = rand() % 100;
      if (r_val < 3)
        P(x, y, TR); /* 3% tree */
      else if (r_val < 7)
        P(x, y, FL); /* 4% flower */
    }
  }

  /* === Barrels and crates near houses (outdoor storage) === */
  for (int y = it; y <= ib; y++) {
    for (int x = il; x <= ir; x++) {
      if (G(x, y) != GF)
        continue;
      /* Check if adjacent to a house wall */
      int adj_wall = 0;
      for (int d = 0; d < 4; d++) {
        int nx = x + (d == 0 ? -1 : d == 1 ? 1 : 0);
        int ny = y + (d == 2 ? -1 : d == 3 ? 1 : 0);
        if (G(nx, ny) == SW)
          adj_wall = 1;
      }
      if (adj_wall && rand() % 12 == 0) {
        P(x, y, (rand() % 2) ? BL : CR);
      }
    }
  }

  /* === Some fenced garden patches === */
  int num_gardens = rr(1, 3 + (int)(density * 3));
  for (int g = 0; g < num_gardens; g++) {
    int gx = rr(il + 3, ir - 6);
    int gy = rr(it + 3, ib - 6);
    int gw = rr(3, 5), gh = rr(3, 5);

    /* Check area is clear */
    int clear = 1;
    for (int y = gy - 1; y <= gy + gh && clear; y++)
      for (int x = gx - 1; x <= gx + gw && clear; x++)
        if (!is_free(x, y))
          clear = 0;

    if (!clear)
      continue;

    /* Place fence border */
    for (int x = gx; x < gx + gw; x++) {
      P(x, gy, FN);
      P(x, gy + gh - 1, FN);
    }
    for (int y = gy; y < gy + gh; y++) {
      P(gx, y, FN);
      P(gx + gw - 1, y, FN);
    }

    /* Fill with flowers and hay */
    for (int y = gy + 1; y < gy + gh - 1; y++)
      for (int x = gx + 1; x < gx + gw - 1; x++)
        P(x, y, (rand() % 3 == 0) ? HY : FL);
  }

  /* === Hay piles near the gate / entrance area === */
  for (int dx = -3; dx <= 3; dx++) {
    int x = W / 2 + dx;
    int y = cb - 3;
    if (is_free(x, y) && rand() % 3 == 0)
      P(x, y, HY);
  }

  /* === Trees outside the city walls === */
  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      if (G(x, y) != GF)
        continue;
      /* Only outside city */
      if (x > cl && x < cr && y > ct && y < cb)
        continue;
      if (y >= river_y)
        continue;

      int r_val = rand() % 100;
      if (r_val < 8)
        P(x, y, TR);
      else if (r_val < 14)
        P(x, y, FL);
    }
  }
}

/* ===== STEP 8: CHURCH (multi-room cathedral with courtyard) ===== */
static void place_church(void) {
  /* Interior of the NAVE (main hall) */
  int ciw = W / 5; if (ciw < 9) ciw = 9; if (ciw > 25) ciw = 25;
  int cih = H / 4; if (cih < 11) cih = 11; if (cih > 35) cih = 35;
  if (ciw % 2 == 0) ciw++;

  int cx1 = W/2 - ciw/2, cy1 = ct + 5;
  int cx2 = cx1 + ciw - 1, cy2 = cy1 + cih - 1;

  /* Verify area clear */
  for (int y = cy1-3; y <= cy2+3; y++)
    for (int x = cx1-3; x <= cx2+3; x++) {
      int t = G(x,y);
      if (t!=GF && t!=FL && t!=CP) return;
    }

  /* ── COURTYARD: cobble plaza around the church ── */
  int yard = 2;
  for (int y = cy1-yard-1; y <= cy2+yard+1; y++)
    for (int x = cx1-yard-1; x <= cx2+yard+1; x++) {
      int t = G(x,y);
      if (t==GF || t==FL) P(x,y,CP);
    }
  /* Fountain left and right of entrance */
  int door_x = (cx1+cx2)/2;
  if (G(door_x-3, cy2+yard) == CP) P(door_x-3, cy2+yard, FO);
  if (G(door_x+3, cy2+yard) == CP) P(door_x+3, cy2+yard, FO);
  /* Courtyard lanterns */
  P(cx1-yard, cy1-yard, LT); P(cx2+yard, cy1-yard, LT);
  P(cx1-yard, cy2+yard, LT); P(cx2+yard, cy2+yard, LT);

  /* ── NAVE floor ── */
  for (int y = cy1; y <= cy2; y++)
    for (int x = cx1; x <= cx2; x++) P(x,y,SF);

  /* ── WALLS and WINDOWS (outer shell) ── */
  int wx1=cx1-1, wy1=cy1-1, wx2=cx2+1, wy2=cy2+1;
  for (int x=wx1;x<=wx2;x++) { P(x,wy1,SW); P(x,wy2,SW); }
  for (int y=wy1;y<=wy2;y++) { P(wx1,y,SW); P(wx2,y,SW); }
  /* Windows every 2 tiles on side walls */
  for (int y=cy1+1; y<=cy2-1; y+=2) { P(wx1,y,WN); P(wx2,y,WN); }
  /* Rose window on back wall */
  P(door_x-1,wy1,WN); P(door_x,wy1,WN); P(door_x+1,wy1,WN);

  /* ── MAIN DOOR ── */
  P(door_x, wy2, WD);
  /* path from door to courtyard road */
  for (int y=wy2+1; y<=cy2+yard+3 && y<cb; y++)
    if (G(door_x,y)==GF||G(door_x,y)==FL||G(door_x,y)==CP) P(door_x,y,CP);

  /* ── NAVE interior ── */
  int aisle_x = door_x;
  /* Pews left and right of aisle */
  int pew_start = cy1+3, pew_end = cy2-4;
  for (int y=pew_start; y<=pew_end; y++) {
    if ((y-pew_start)%3==2) continue;
    for (int x=cx1+1; x<aisle_x-1; x++) if(G(x,y)==SF) P(x,y,PW);
    for (int x=aisle_x+2; x<=cx2-1; x++) if(G(x,y)==SF) P(x,y,PW);
  }
  /* Torches along side walls */
  for (int y=cy1+2; y<=cy2-2; y+=3) { P(cx1,y,FT); P(cx2,y,FT); }

  /* ── APSE: semicircular altar area at the front ── */
  int apse_y = cy1+1;
  P(aisle_x,   apse_y, AL);   /* main altar */
  P(aisle_x-1, apse_y, FT);   /* flanking torches */
  P(aisle_x+1, apse_y, FT);
  /* Gold artifacts flanking */
  if (ciw>=11) {
    P(aisle_x-2, apse_y, GA);
    P(aisle_x+2, apse_y, GA);
  }
  /* Bishop throne */
  P(aisle_x, apse_y+1, CH);

  /* ── SIDE ROOMS (left sacristy + right treasury) ── */
  int rm_h = (cih >= 16) ? 5 : 4;
  int rm_w = (ciw >= 17) ? 5 : 4;
  /* Left sacristy */
  int lrx1=cx1-rm_w-1, lry1=cy1+2, lrx2=cx1-2, lry2=cy1+2+rm_h-1;
  int space_ok=1;
  for (int y=lry1-1;y<=lry2+1&&space_ok;y++)
    for (int x=lrx1-1;x<=lrx2+1&&space_ok;x++)
      if(G(x,y)!=GF&&G(x,y)!=FL&&G(x,y)!=CP&&G(x,y)!=CW) space_ok=0;
  if (space_ok && lrx1>cl+3) {
    for (int y=lry1;y<=lry2;y++) for (int x=lrx1;x<=lrx2;x++) P(x,y,SF);
    for (int x=lrx1-1;x<=lrx2+1;x++){P(x,lry1-1,SW);P(x,lry2+1,SW);}
    for (int y=lry1-1;y<=lry2+1;y++){P(lrx1-1,y,SW);P(lrx2+1,y,SW);}
    /* Door connecting to nave */
    P(lrx2+1, lry1+rm_h/2, WD);
    /* Furnish: bookshelf, gold artifact, torch */
    P(lrx1,lry1,BK); P(lrx2,lry1,GA); P(lrx1,lry2,FT);
    P(lrx1+1,lry2,CB); /* chest */
  }
  /* Right treasury */
  int rrx1=cx2+2, rry1=cy1+2, rrx2=cx2+rm_w+1, rry2=cy1+2+rm_h-1;
  space_ok=1;
  for (int y=rry1-1;y<=rry2+1&&space_ok;y++)
    for (int x=rrx1-1;x<=rrx2+1&&space_ok;x++)
      if(G(x,y)!=GF&&G(x,y)!=FL&&G(x,y)!=CP&&G(x,y)!=CW) space_ok=0;
  if (space_ok && rrx2<cr-3) {
    for (int y=rry1;y<=rry2;y++) for (int x=rrx1;x<=rrx2;x++) P(x,y,SF);
    for (int x=rrx1-1;x<=rrx2+1;x++){P(x,rry1-1,SW);P(x,rry2+1,SW);}
    for (int y=rry1-1;y<=rry2+1;y++){P(rrx1-1,y,SW);P(rrx2+1,y,SW);}
    /* Door connecting to nave */
    P(rrx1-1, rry1+rm_h/2, WD);
    /* Furnish: gold artifacts, chests, torch */
    P(rrx1,rry1,GA); P(rrx2,rry1,GA);
    P(rrx1,rry2,CB); P(rrx2,rry2,CB);
    P(rrx1+rm_w/2,rry1+rm_h/2,FT);
  }
  /* ── BELL TOWERS: flanking the entrance ── */
  if (ciw >= 13) {
    int tw=3, th=3;
    /* Left tower */
    int ltx1=cx1-tw, lty1=cy2-th-1, ltx2=cx1-1, lty2=cy2;
    int tok=1;
    for(int y=lty1-1;y<=lty2+1&&tok;y++) for(int x=ltx1-1;x<=ltx2+1&&tok;x++)
      if(G(x,y)!=GF&&G(x,y)!=FL&&G(x,y)!=CP&&G(x,y)!=CW) tok=0;
    if(tok && ltx1>cl+3) {
      for(int y=lty1;y<=lty2;y++) for(int x=ltx1;x<=ltx2;x++) P(x,y,SF);
      for(int x=ltx1-1;x<=ltx2+1;x++){P(x,lty1-1,SW);P(x,lty2+1,SW);}
      for(int y=lty1-1;y<=lty2+1;y++){P(ltx1-1,y,SW);P(ltx2+1,y,SW);}
      P(ltx1-1,lty1+1,WD); /* door to courtyard */
      P(ltx1+(tw/2), lty1, FT);
    }
    /* Right tower */
    int rtx1=cx2+1, rty1=cy2-th-1, rtx2=cx2+tw, rty2=cy2;
    tok=1;
    for(int y=rty1-1;y<=rty2+1&&tok;y++) for(int x=rtx1-1;x<=rtx2+1&&tok;x++)
      if(G(x,y)!=GF&&G(x,y)!=FL&&G(x,y)!=CP&&G(x,y)!=CW) tok=0;
    if(tok && rtx2<cr-3) {
      for(int y=rty1;y<=rty2;y++) for(int x=rtx1;x<=rtx2;x++) P(x,y,SF);
      for(int x=rtx1-1;x<=rtx2+1;x++){P(x,rty1-1,SW);P(x,rty2+1,SW);}
      for(int y=rty1-1;y<=rty2+1;y++){P(rtx1-1,y,SW);P(rtx2+1,y,SW);}
      P(rtx2+1,rty1+1,WD); /* door to courtyard */
      P(rtx1+(tw/2), rty1, FT);
    }
  }

  printf("[INFO] Church placed: %dx%d nave at (%d,%d), with sacristy+treasury+towers\n",
         ciw, cih, cx1, cy1);
}

/* ===== STEP 9: SPECIAL BUILDINGS ===== */
/* Place a blacksmith (forge building) near the gate area */
static void place_special_buildings(void) {
  /* Try to place a forge near the entrance */
  int fx = W / 2 + rr(5, 8);
  int fy = cb - rr(4, 7);

  if (try_place_rect_house(fx, fy, 4, 3)) {
    /* Replace some furniture with forge items */
    for (int y = fy; y < fy + 3; y++)
      for (int x = fx; x < fx + 4; x++) {
        int t = G(x, y);
        if (t == TB || t == BB)
          P(x, y, FG);
        if (t == BK)
          P(x, y, CB);
      }
    /* Mark with an anvil if there's a wood floor tile */
    if (G(fx + 1, fy + 1) == WF || G(fx + 1, fy + 1) == WT)
      P(fx + 1, fy + 1, FG);
  }
}

/* ===== OUTPUT ===== */
static void print_map_formatted(void) {
  printf("\n=== GENERATED MAP (%dx%d) ===\n\n", W, H);
  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      if (x > 0)
        printf(", ");
      printf("%s", TSTR[map[y * W + x]]);
    }
    printf("\n");
  }
}

static void save_map(const char *filename) {
  FILE *f = fopen(filename, "w");
  if (!f) {
    fprintf(stderr, "Error: cannot open '%s' for writing\n", filename);
    return;
  }
  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      fprintf(f, "%s", TSTR[map[y * W + x]]);
    }
    fprintf(f, "\n");
  }
  fclose(f);
  printf("[INFO] Map saved to '%s'\n", filename);
}

/* ===== MAIN ===== */
static void print_usage(const char *prog) {
  printf("Usage: %s <width> <height> <density> <min_house> <max_house> "
         "<output.txt>\n",
         prog);
  printf("\n");
  printf("  width      Map width in tiles (min 30)\n");
  printf("  height     Map height in tiles (min 25)\n");
  printf("  density    House density 0.1 (sparse) to 0.9 (packed)\n");
  printf("  min_house  Minimum house interior area in tiles (e.g. 6)\n");
  printf("  max_house  Maximum house interior area in tiles (e.g. 36)\n");
  printf("  output     Output filename (.txt)\n");
  printf("\n");
  printf("Example: %s 80 60 0.4 9 36 city.txt\n", prog);
}

int main(int argc, char **argv) {
  if (argc < 7) {
    print_usage(argv[0]);
    return 1;
  }

  W = atoi(argv[1]);
  H = atoi(argv[2]);
  density = atof(argv[3]);
  min_area = atoi(argv[4]);
  max_area = atoi(argv[5]);
  const char *outfile = argv[6];

  /* Validate inputs */
  if (W < 30 || H < 25) {
    fprintf(stderr, "Error: Map must be at least 30x25\n");
    return 1;
  }
  if (density < 0.05f || density > 0.95f) {
    fprintf(stderr, "Error: Density must be between 0.05 and 0.95\n");
    return 1;
  }
  if (min_area < 4) {
    fprintf(stderr, "Error: min_house must be at least 4\n");
    return 1;
  }
  if (max_area < min_area) {
    fprintf(stderr, "Error: max_house must be >= min_house\n");
    return 1;
  }
  if (max_area > 100) {
    fprintf(stderr, "Warning: Very large max_house, capping at 100\n");
    max_area = 100;
  }

  srand((unsigned)time(NULL));

  printf("[INFO] Generating %dx%d map, density=%.2f, house=%d-%d tiles\n", W, H,
         density, min_area, max_area);

  /* === GENERATION PIPELINE === */
  init_map();

  printf("[STEP 1] Placing river...\n");
  place_river();

  printf("[STEP 2] Building city walls...\n");
  place_walls();

  printf("[STEP 3] Laying road network...\n");
  generate_roads();

  printf("[STEP 4] Creating town plaza...\n");
  generate_plaza();

  printf("[STEP 5] Building docks...\n");
  place_docks();

  printf("[STEP 6] Carving river inlet...\n");
  place_inlet();

  printf("[STEP 7] Building church...\n");
  place_church();

  printf("[STEP 8] Generating houses...\n");
  generate_houses();

  printf("[STEP 9] Placing special buildings...\n");
  place_special_buildings();

  printf("[STEP 10] Adding decorations...\n");
  add_decorations();

  /* === OUTPUT === */
  print_map_formatted();
  save_map(outfile);

  printf("\n[DONE] Map generation complete!\n");
  printf("  Tile legend:\n");
  for (int i = 0; i < TILE_COUNT; i++) {
    const char *desc[] = {
        "Grass Floor", "Cobble Path", "Stone Wall",  "Wood Floor",
        "Wood Table",  "Fire Torch",  "River Water", "Wood Door",
        "Top Bed",     "Bottom Bed",  "Chair",       "Bookshelf",
        "Crate",       "City Wall",   "Well",        "Tree",
        "Flower",      "Barrel",      "Lantern",     "Market Stall",
        "Sand/Shore",  "Gate",        "Chest",       "Window",
        "Hay",         "Forge",       "Stool",       "Fence",
        "Stone Floor", "Altar",       "Pew",         "Dock",
        "Pier Post",   "Boat",        "Bench",       "Gold Artifact",
        "Fountain"};
    printf("    %s = %s\n", TSTR[i], desc[i]);
  }

  free(map);
  return 0;
}
