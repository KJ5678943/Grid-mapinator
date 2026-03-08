#include <SDL.h>
#include <jpeglib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int build_default_output_path(const char *input, char *output,
                                     size_t output_size) {
  const char *dot = strrchr(input, '.');
  size_t base_len = dot ? (size_t)(dot - input) : strlen(input);
  const char *ext = ".jpg";

  if (base_len + strlen(ext) + 1 > output_size)
    return 0;

  memcpy(output, input, base_len);
  output[base_len] = '\0';
  strcat(output, ext);
  return 1;
}

static void print_usage(const char *prog) {
  fprintf(stderr,
          "Usage: %s <input.bmp> [output.jpg] [quality]\n"
          "  quality: 1-100 (default: 92)\n",
          prog);
}

int main(int argc, char **argv) {
  const int default_quality = 92;
  int quality = default_quality;
  char auto_output[4096];
  const char *input_path;
  const char *output_path;

  SDL_Surface *bmp = NULL;
  SDL_Surface *rgb = NULL;
  FILE *out = NULL;
  int locked = 0;
  int ok = 0;

  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;

  if (argc < 2 || argc > 4) {
    print_usage(argv[0]);
    return 1;
  }

  input_path = argv[1];
  if (argc >= 3) {
    output_path = argv[2];
  } else {
    if (!build_default_output_path(input_path, auto_output,
                                   sizeof(auto_output))) {
      fprintf(stderr, "Error: output path is too long\n");
      return 1;
    }
    output_path = auto_output;
  }

  if (argc == 4) {
    quality = atoi(argv[3]);
    if (quality < 1 || quality > 100) {
      fprintf(stderr, "Error: quality must be between 1 and 100\n");
      return 1;
    }
  }

  bmp = SDL_LoadBMP(input_path);
  if (!bmp) {
    fprintf(stderr, "Error: failed to load BMP '%s': %s\n", input_path,
            SDL_GetError());
    goto cleanup;
  }

  rgb = SDL_ConvertSurfaceFormat(bmp, SDL_PIXELFORMAT_RGB24, 0);
  if (!rgb) {
    fprintf(stderr, "Error: failed to convert BMP to RGB24: %s\n",
            SDL_GetError());
    goto cleanup;
  }

  out = fopen(output_path, "wb");
  if (!out) {
    fprintf(stderr, "Error: failed to open output '%s'\n", output_path);
    goto cleanup;
  }

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  jpeg_stdio_dest(&cinfo, out);
  cinfo.image_width = rgb->w;
  cinfo.image_height = rgb->h;
  cinfo.input_components = 3;
  cinfo.in_color_space = JCS_RGB;
  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, 1);
  jpeg_start_compress(&cinfo, 1);

  if (SDL_MUSTLOCK(rgb)) {
    if (SDL_LockSurface(rgb) != 0) {
      fprintf(stderr, "Error: failed to lock RGB surface: %s\n",
              SDL_GetError());
      jpeg_destroy_compress(&cinfo);
      goto cleanup;
    }
    locked = 1;
  }

  while (cinfo.next_scanline < cinfo.image_height) {
    JSAMPROW row = (JSAMPROW)((unsigned char *)rgb->pixels +
                              cinfo.next_scanline * rgb->pitch);
    jpeg_write_scanlines(&cinfo, &row, 1);
  }

  if (locked) {
    SDL_UnlockSurface(rgb);
    locked = 0;
  }

  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);

  printf("Converted '%s' -> '%s' (quality=%d)\n", input_path, output_path,
         quality);
  ok = 1;

cleanup:
  if (out)
    fclose(out);
  if (rgb)
    SDL_FreeSurface(rgb);
  if (bmp)
    SDL_FreeSurface(bmp);

  return ok ? 0 : 1;
}
