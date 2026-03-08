/*
 * ============================================================================
 *  MAP VIEWER - SDL2 Renderer
 * ============================================================================
 *  Loads a .txt map file produced by mapgen and renders it with colored tiles.
 *
 *  Usage:  ./mapview <map.txt>
 *  Example: ./mapview city.txt
 *
 *  Controls:
 *    Arrow Keys / WASD  - Pan the camera
 *    +/- or Scroll      - Zoom in/out
 *    R                  - Reset view (fit map to window)
 *    F                  - Toggle fullscreen
 *    G                  - Toggle grid lines
 *    P                  - Save screenshot
 *    ESC / Q            - Quit
 *    Click + Drag       - Pan with mouse
 *
 *  Compile:
 *    gcc -o mapview mapview.c $(sdl2-config --cflags --libs) -lm
 * ============================================================================
 */

#include <SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== TILE DEFINITIONS (must match mapgen.c) ===== */
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

static const char *TILE_CODES[] = {
    "GF", "CP", "SW", "WF", "WT", "FT", "RW", "WD", "TB", "BB",
    "CH", "BK", "CR", "CW", "WL", "TR", "FL", "BL", "LT", "MK",
    "SN", "GT", "CB", "WN", "HY", "FG", "ST", "FN", "SF", "AL",
    "PW", "DK", "PL", "BT", "BN", "GA", "FO"};

static const char *TILE_NAMES[] = {
    "Grass Floor", "Cobble Path", "Stone Wall", "Wood Floor",  "Wood Table",
    "Fire Torch",  "River Water", "Wood Door",  "Top Bed",     "Bottom Bed",
    "Chair",       "Bookshelf",   "Crate",      "City Wall",   "Well",
    "Tree",        "Flower",      "Barrel",     "Lantern",     "Market Stall",
    "Sand/Shore",  "Gate",        "Chest",      "Window",      "Hay",
    "Forge",       "Stool",       "Fence",      "Stone Floor", "Altar",
    "Pew",         "Dock",        "Pier Post"};

/* RGB colors for each tile type */
typedef struct {
  Uint8 r, g, b;
} Color;

static const Color TILE_COLORS[] = {
    /* GF */ {86, 152, 58},   /* green grass */
    /* CP */ {160, 145, 120}, /* cobblestone beige */
    /* SW */ {110, 105, 100}, /* grey stone wall */
    /* WF */ {170, 120, 70},  /* warm wood brown */
    /* WT */ {130, 85, 45},   /* dark wood table */
    /* FT */ {255, 170, 40},  /* orange torch flame */
    /* RW */ {50, 100, 180},  /* blue water */
    /* WD */ {140, 90, 40},   /* brown door */
    /* TB */ {180, 50, 50},   /* red bed top */
    /* BB */ {160, 45, 45},   /* red bed bottom */
    /* CH */ {150, 100, 55},  /* chair wood */
    /* BK */ {90, 60, 30},    /* dark bookshelf */
    /* CR */ {165, 130, 80},  /* crate tan */
    /* CW */ {75, 75, 80},    /* dark city wall */
    /* WL */ {90, 140, 200},  /* well blue-stone */
    /* TR */ {40, 110, 35},   /* dark green tree */
    /* FL */ {220, 100, 140}, /* pink flower */
    /* BL */ {120, 80, 40},   /* barrel brown */
    /* LT */ {255, 220, 100}, /* yellow lantern */
    /* MK */ {200, 160, 90},  /* market stall tan */
    /* SN */ {210, 195, 150}, /* sandy beige */
    /* GT */ {130, 110, 80},  /* gate wood */
    /* CB */ {160, 120, 60},  /* chest gold-brown */
    /* WN */ {140, 180, 220}, /* light blue window */
    /* HY */ {200, 190, 80},  /* yellow hay */
    /* FG */ {200, 80, 40},   /* red-orange forge */
    /* ST */ {155, 110, 65},  /* stool wood */
    /* FN */ {140, 115, 70},  /* fence wood */
    /* SF */ {130, 130, 140}, /* stone floor grey */
    /* AL */ {200, 180, 50},  /* golden altar */
    /* PW */ {100, 70, 45},   /* dark wood pew */
    /* DK */ {160, 120, 60},  /* warm dock planks */
    /* PL */ {90, 70, 50},    /* dark pier post */
    /* BT */ {70, 120, 160},  /* blue-grey boat */
    /* BN */ {140, 100, 55},  /* bench wood */
    /* GA */ {230, 200, 50},  /* gleaming gold artifact */
    /* FO */ {80, 170, 210},  /* fountain water blue */
};

/* ===== MAP DATA ===== */
static int map_w = 0, map_h = 0;
static int *map_data = NULL;

/* ===== MAP LOADING ===== */

/* Lookup tile code (2-char string) -> tile enum index */
static int tile_from_code(const char *code) {
  for (int i = 0; i < TILE_COUNT; i++) {
    if (code[0] == TILE_CODES[i][0] && code[1] == TILE_CODES[i][1])
      return i;
  }
  return GF; /* default to grass if unknown */
}

/* First pass: determine map dimensions */
static int measure_map(const char *filename) {
  FILE *f = fopen(filename, "r");
  if (!f) {
    fprintf(stderr, "Error: cannot open '%s'\n", filename);
    return 0;
  }

  char line[65536];
  map_h = 0;
  map_w = 0;

  while (fgets(line, sizeof(line), f)) {
    int len = (int)strlen(line);
    /* Strip trailing newline/CR */
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
      line[--len] = '\0';

    if (len == 0)
      continue;

    /* Each tile is 2 characters, no separators */
    int w = len / 2;
    if (w > map_w)
      map_w = w;
    map_h++;
  }

  fclose(f);
  return (map_w > 0 && map_h > 0);
}

/* Second pass: load tile data */
static int load_map(const char *filename) {
  if (!measure_map(filename))
    return 0;

  map_data = calloc(map_w * map_h, sizeof(int));
  if (!map_data) {
    fprintf(stderr, "Error: out of memory for %dx%d map\n", map_w, map_h);
    return 0;
  }

  FILE *f = fopen(filename, "r");
  if (!f)
    return 0;

  char line[65536];
  int row = 0;

  while (fgets(line, sizeof(line), f) && row < map_h) {
    int len = (int)strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
      line[--len] = '\0';

    if (len == 0)
      continue;

    int cols = len / 2;
    for (int col = 0; col < cols && col < map_w; col++) {
      char code[3] = {line[col * 2], line[col * 2 + 1], '\0'};
      map_data[row * map_w + col] = tile_from_code(code);
    }
    row++;
  }

  fclose(f);
  printf("[INFO] Loaded map: %d x %d tiles\n", map_w, map_h);
  return 1;
}

/* ===== RENDERER ===== */

#define INITIAL_WIN_W 1200
#define INITIAL_WIN_H 800
#define MIN_TILE_SIZE 1
#define MAX_TILE_SIZE 64
#define MAP_SHOT_TILE_SIZE 8

static int save_full_map_screenshot(const char *output_path) {
  int img_w = map_w * MAP_SHOT_TILE_SIZE;
  int img_h = map_h * MAP_SHOT_TILE_SIZE;
  if (img_w <= 0 || img_h <= 0) {
    fprintf(stderr, "[ERROR] Invalid map size for screenshot (%dx%d)\n", map_w,
            map_h);
    return 0;
  }

  SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(
      0, img_w, img_h, 32, SDL_PIXELFORMAT_ARGB8888);
  if (!surface) {
    fprintf(stderr, "[ERROR] Failed to create map screenshot surface: %s\n",
            SDL_GetError());
    return 0;
  }

  for (int row = 0; row < map_h; row++) {
    for (int col = 0; col < map_w; col++) {
      int tile = map_data[row * map_w + col];
      if (tile < 0 || tile >= TILE_COUNT)
        tile = GF;

      Color c = TILE_COLORS[tile];
      Uint32 pixel = SDL_MapRGB(surface->format, c.r, c.g, c.b);
      SDL_Rect r = {col * MAP_SHOT_TILE_SIZE, row * MAP_SHOT_TILE_SIZE,
                    MAP_SHOT_TILE_SIZE, MAP_SHOT_TILE_SIZE};
      SDL_FillRect(surface, &r, pixel);
    }
  }

  char filename[64];
  const char *target = output_path;
  if (!target || target[0] == '\0') {
    snprintf(filename, sizeof(filename), "map_%u.bmp", SDL_GetTicks());
    target = filename;
  }

  if (SDL_SaveBMP(surface, target) != 0) {
    fprintf(stderr, "[ERROR] Failed to save map screenshot: %s\n",
            SDL_GetError());
    SDL_FreeSurface(surface);
    return 0;
  }

  SDL_FreeSurface(surface);
  printf("[INFO] Saved full map image to %s (%dx%d)\n", target, img_w, img_h);
  return 1;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: %s <map.txt> [--export-bmp <output.bmp>]\n", argv[0]);
    printf("Example: %s city.txt\n", argv[0]);
    printf("         %s city.txt --export-bmp city.bmp\n", argv[0]);
    return 1;
  }

  const char *map_path = argv[1];
  const char *export_bmp = NULL;
  for (int i = 2; i < argc; i++) {
    if (strcmp(argv[i], "--export-bmp") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "Error: --export-bmp requires an output path\n");
        return 1;
      }
      export_bmp = argv[++i];
    } else {
      fprintf(stderr, "Error: unknown argument '%s'\n", argv[i]);
      return 1;
    }
  }

  /* Load map */
  if (!load_map(map_path)) {
    fprintf(stderr, "Failed to load map '%s'\n", map_path);
    return 1;
  }

  if (export_bmp) {
    int ok = save_full_map_screenshot(export_bmp);
    free(map_data);
    return ok ? 0 : 1;
  }

  /* Init SDL */
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
    return 1;
  }

  char title[256];
  snprintf(title, sizeof(title), "Map Viewer - %s (%dx%d)", map_path, map_w,
           map_h);

  SDL_Window *win = SDL_CreateWindow(
      title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, INITIAL_WIN_W,
      INITIAL_WIN_H, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

  if (!win) {
    fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_Renderer *ren = SDL_CreateRenderer(
      win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  if (!ren) {
    fprintf(stderr, "SDL_CreateRenderer error: %s\n", SDL_GetError());
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 1;
  }

  /* Camera state */
  int win_w = INITIAL_WIN_W, win_h = INITIAL_WIN_H;

  /* Calculate initial tile size to fit the map in window */
  float tile_size;
  {
    float fit_w = (float)win_w / (float)map_w;
    float fit_h = (float)win_h / (float)map_h;
    tile_size = (fit_w < fit_h) ? fit_w : fit_h;
    if (tile_size < MIN_TILE_SIZE)
      tile_size = MIN_TILE_SIZE;
  }

  /* Camera offset (top-left corner of viewport in tile-space) */
  float cam_x = 0.0f, cam_y = 0.0f;

  /* Center the map */
  float total_w = map_w * tile_size;
  float total_h = map_h * tile_size;
  cam_x = -(win_w - total_w) / 2.0f;
  cam_y = -(win_h - total_h) / 2.0f;

  int show_grid = 0;
  int is_fullscreen = 0;
  int take_screenshot = 0;
  int dragging = 0;
  int drag_start_x = 0, drag_start_y = 0;
  float drag_cam_x = 0, drag_cam_y = 0;
  float pan_speed = 400.0f; /* pixels per second */

  /* Track which keys are held */
  int key_up = 0, key_down = 0, key_left = 0, key_right = 0;

  Uint32 last_ticks = SDL_GetTicks();
  int running = 1;

  /* Tooltip state */
  int hover_tx = -1, hover_ty = -1;

  printf("[INFO] Viewer ready. Controls:\n");
  printf("  Arrows/WASD = Pan | +/-/Scroll = Zoom | R = Reset | G = Grid | F = "
         "Fullscreen\n"
         "  P = Screenshot | ESC = Quit\n");

  while (running) {
    Uint32 now = SDL_GetTicks();
    float dt = (now - last_ticks) / 1000.0f;
    last_ticks = now;

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      switch (e.type) {
      case SDL_QUIT:
        running = 0;
        break;

      case SDL_KEYDOWN:
        switch (e.key.keysym.sym) {
        case SDLK_ESCAPE:
        case SDLK_q:
          running = 0;
          break;
        case SDLK_UP:
        case SDLK_w:
          key_up = 1;
          break;
        case SDLK_DOWN:
        case SDLK_s:
          key_down = 1;
          break;
        case SDLK_LEFT:
        case SDLK_a:
          key_left = 1;
          break;
        case SDLK_RIGHT:
        case SDLK_d:
          key_right = 1;
          break;
        case SDLK_EQUALS:
        case SDLK_PLUS:
        case SDLK_KP_PLUS: {
          /* Zoom in toward center */
          float cx = cam_x + win_w / 2.0f;
          float cy = cam_y + win_h / 2.0f;
          tile_size *= 1.25f;
          if (tile_size > MAX_TILE_SIZE)
            tile_size = MAX_TILE_SIZE;
          cam_x = cx - win_w / 2.0f;
          cam_y = cy - win_h / 2.0f;
          break;
        }
        case SDLK_MINUS:
        case SDLK_KP_MINUS: {
          float cx = cam_x + win_w / 2.0f;
          float cy = cam_y + win_h / 2.0f;
          tile_size /= 1.25f;
          if (tile_size < MIN_TILE_SIZE)
            tile_size = MIN_TILE_SIZE;
          cam_x = cx - win_w / 2.0f;
          cam_y = cy - win_h / 2.0f;
          break;
        }
        case SDLK_r: {
          /* Reset: fit map to window */
          float fit_w = (float)win_w / (float)map_w;
          float fit_h = (float)win_h / (float)map_h;
          tile_size = (fit_w < fit_h) ? fit_w : fit_h;
          if (tile_size < MIN_TILE_SIZE)
            tile_size = MIN_TILE_SIZE;
          total_w = map_w * tile_size;
          total_h = map_h * tile_size;
          cam_x = -(win_w - total_w) / 2.0f;
          cam_y = -(win_h - total_h) / 2.0f;
          break;
        }
        case SDLK_g:
          show_grid = !show_grid;
          break;
        case SDLK_f:
          is_fullscreen = !is_fullscreen;
          SDL_SetWindowFullscreen(
              win, is_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
          break;
        case SDLK_p:
          take_screenshot = 1;
          break;
        default:
          break;
        }
        break;

      case SDL_KEYUP:
        switch (e.key.keysym.sym) {
        case SDLK_UP:
        case SDLK_w:
          key_up = 0;
          break;
        case SDLK_DOWN:
        case SDLK_s:
          key_down = 0;
          break;
        case SDLK_LEFT:
        case SDLK_a:
          key_left = 0;
          break;
        case SDLK_RIGHT:
        case SDLK_d:
          key_right = 0;
          break;
        default:
          break;
        }
        break;

      case SDL_MOUSEWHEEL: {
        /* Zoom toward mouse cursor */
        int mx, my;
        SDL_GetMouseState(&mx, &my);
        float world_x = cam_x + mx;
        float world_y = cam_y + my;

        if (e.wheel.y > 0)
          tile_size *= 1.15f;
        else if (e.wheel.y < 0)
          tile_size /= 1.15f;

        if (tile_size < MIN_TILE_SIZE)
          tile_size = MIN_TILE_SIZE;
        if (tile_size > MAX_TILE_SIZE)
          tile_size = MAX_TILE_SIZE;

        /* Adjust camera so the point under cursor stays fixed */
        float tile_x =
            world_x /
            ((cam_x + mx) >= 0 ? (tile_size / (tile_size / 1.15f)) : tile_size);
        (void)tile_x;
        /* Simpler: just re-center on the mouse world point */
        /* Actually let's keep it centered on mouse */
        float new_world_x = (world_x / (e.wheel.y > 0 ? tile_size / 1.15f
                                                      : tile_size * 1.15f)) *
                            tile_size;
        float new_world_y = (world_y / (e.wheel.y > 0 ? tile_size / 1.15f
                                                      : tile_size * 1.15f)) *
                            tile_size;
        cam_x += (new_world_x - world_x);
        cam_y += (new_world_y - world_y);
        break;
      }

      case SDL_MOUSEBUTTONDOWN:
        if (e.button.button == SDL_BUTTON_LEFT) {
          dragging = 1;
          drag_start_x = e.button.x;
          drag_start_y = e.button.y;
          drag_cam_x = cam_x;
          drag_cam_y = cam_y;
        }
        break;

      case SDL_MOUSEBUTTONUP:
        if (e.button.button == SDL_BUTTON_LEFT)
          dragging = 0;
        break;

      case SDL_MOUSEMOTION:
        if (dragging) {
          cam_x = drag_cam_x - (e.motion.x - drag_start_x);
          cam_y = drag_cam_y - (e.motion.y - drag_start_y);
        }
        /* Track hover for tooltip */
        {
          int tx = (int)((cam_x + e.motion.x) / tile_size);
          int ty = (int)((cam_y + e.motion.y) / tile_size);
          if (tx >= 0 && tx < map_w && ty >= 0 && ty < map_h) {
            hover_tx = tx;
            hover_ty = ty;
          } else {
            hover_tx = -1;
            hover_ty = -1;
          }
        }
        break;

      case SDL_WINDOWEVENT:
        if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
          win_w = e.window.data1;
          win_h = e.window.data2;
        }
        break;
      }
    }

    /* Keyboard panning */
    float move = pan_speed * dt / tile_size * tile_size;
    if (key_up)
      cam_y -= move;
    if (key_down)
      cam_y += move;
    if (key_left)
      cam_x -= move;
    if (key_right)
      cam_x += move;

    /* === RENDER === */
    SDL_SetRenderDrawColor(ren, 20, 20, 25, 255);
    SDL_RenderClear(ren);

    /* Calculate visible tile range */
    int start_col = (int)(cam_x / tile_size);
    int start_row = (int)(cam_y / tile_size);
    int end_col = (int)((cam_x + win_w) / tile_size) + 1;
    int end_row = (int)((cam_y + win_h) / tile_size) + 1;

    if (start_col < 0)
      start_col = 0;
    if (start_row < 0)
      start_row = 0;
    if (end_col > map_w)
      end_col = map_w;
    if (end_row > map_h)
      end_row = map_h;

    /* Draw visible tiles */
    for (int row = start_row; row < end_row; row++) {
      for (int col = start_col; col < end_col; col++) {
        int tile = map_data[row * map_w + col];
        if (tile < 0 || tile >= TILE_COUNT)
          tile = GF;

        Color c = TILE_COLORS[tile];

        SDL_Rect r;
        r.x = (int)(col * tile_size - cam_x);
        r.y = (int)(row * tile_size - cam_y);
        r.w = (int)(tile_size + 1); /* +1 to avoid gaps */
        r.h = (int)(tile_size + 1);

        SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, 255);
        SDL_RenderFillRect(ren, &r);
      }
    }

    /* Draw grid lines (only if zoomed in enough) */
    if (show_grid && tile_size >= 6) {
      SDL_SetRenderDrawColor(ren, 0, 0, 0, 40);

      for (int col = start_col; col <= end_col; col++) {
        int x = (int)(col * tile_size - cam_x);
        SDL_RenderDrawLine(ren, x, 0, x, win_h);
      }
      for (int row = start_row; row <= end_row; row++) {
        int y = (int)(row * tile_size - cam_y);
        SDL_RenderDrawLine(ren, 0, y, win_w, y);
      }
    }

    /* Draw tile info in window title when hovering */
    if (hover_tx >= 0 && hover_ty >= 0) {
      int tile = map_data[hover_ty * map_w + hover_tx];
      snprintf(title, sizeof(title),
               "Map Viewer - %s (%dx%d) | Tile (%d,%d): %s [%s] | Zoom: %.1fx",
               map_path, map_w, map_h, hover_tx, hover_ty, TILE_CODES[tile],
               TILE_NAMES[tile], tile_size);
      SDL_SetWindowTitle(win, title);
    }

    if (take_screenshot) {
      save_full_map_screenshot(NULL);
      take_screenshot = 0;
    }

    SDL_RenderPresent(ren);
  }

  /* Cleanup */
  free(map_data);
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();

  return 0;
}
