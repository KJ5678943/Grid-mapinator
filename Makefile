CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra

SDL2_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null)
SDL2_LIBS := $(shell sdl2-config --libs 2>/dev/null)
JPEG_CFLAGS := $(shell pkg-config --cflags libjpeg 2>/dev/null)
JPEG_LIBS := $(shell pkg-config --libs libjpeg 2>/dev/null)

BIN_DIR := Sauce
IMAGES_DIR := map_preview
TXT_DIR := map_txt
SRC_DIR := Source

MAPGEN_BIN := $(BIN_DIR)/mapgen
MAPGEN_SRC := $(SRC_DIR)/mapgen.c
MAPVIEW_BIN := $(BIN_DIR)/mapview
MAPVIEW_SRC := $(SRC_DIR)/mapview.c
BMP2JPG_BIN := $(BIN_DIR)/bmp_to_jpg
BMP2JPG_SRC := $(SRC_DIR)/bmp_to_jpg.c

.PHONY: all clean run-mapview dirs

all: dirs $(MAPGEN_BIN) $(MAPVIEW_BIN) $(BMP2JPG_BIN)

dirs:
	mkdir -p $(BIN_DIR) $(IMAGES_DIR) $(TXT_DIR) $(SRC_DIR)

$(MAPGEN_BIN): $(MAPGEN_SRC)
	$(CC) $(CFLAGS) $< -o $@

$(MAPVIEW_BIN): $(MAPVIEW_SRC)
	$(CC) $(CFLAGS) $(SDL2_CFLAGS) $< -o $@ $(SDL2_LIBS) -lm

$(BMP2JPG_BIN): $(BMP2JPG_SRC)
	$(CC) $(CFLAGS) $(SDL2_CFLAGS) $(JPEG_CFLAGS) $< -o $@ $(SDL2_LIBS) $(JPEG_LIBS)

clean:
	rm -f $(MAPGEN_BIN) $(MAPVIEW_BIN) $(BMP2JPG_BIN)

run-mapview: $(MAPVIEW_BIN)
	@if [ -z "$(MAP)" ]; then \
		echo "Usage: make run-mapview MAP=path/to/map.txt"; \
		exit 1; \
	fi
	$(MAPVIEW_BIN) "$(MAP)"
