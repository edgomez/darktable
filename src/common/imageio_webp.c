/*
    This file is part of darktable,
    copyright (c) 2018 edouard gomez.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    darktable is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with darktable.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "common/imageio.h"
#include "common/imageio_webp.h"

#include "webp/decode.h"
#include <stdio.h>

static int is_webp(const char *filename)
{
  int ret = 0;
  FILE* f = g_fopen(filename, "rb");
  if (!f) goto exit;

  char buf[4 + 4 + 4];
  if (sizeof(buf) != fread(buf, 1, sizeof(buf), f)) goto exit;

  ret = !!(   buf[0] == 'R' && buf[1] == 'I' && buf[2] == 'F' && buf[3] == 'F'
           && buf[8] == 'W' && buf[9] == 'E' && buf[10] == 'B' && buf[11] == 'P');

exit:
  if (f) {
    fclose(f);
    f = NULL;
  }
  return ret;
}

dt_imageio_retval_t dt_imageio_open_webp(dt_image_t *img, const char *filename, dt_mipmap_buffer_t *mbuf)
{
  // reject non webp files
  if(!is_webp(filename))
    return DT_IMAGEIO_FILE_CORRUPTED;

  void *filebuf = NULL;
  uint8_t *rgb = NULL;
  FILE *f = g_fopen(filename, "rb");
  if(!f) return DT_IMAGEIO_FILE_CORRUPTED;

  if (fseek(f, 0, SEEK_END)) goto error_file_corrupted;
  long int filelen = ftell(f);
  filebuf = malloc(filelen);
  if (!filebuf)  goto error_file_corrupted;
  if (fseek(f, 0, SEEK_SET)) goto error_file_corrupted;
  if (filelen != fread(filebuf, 1, filelen, f)) goto error_file_corrupted;
  fclose(f);
  f = NULL;

  int w, h;
  rgb = WebPDecodeRGB(filebuf, filelen, &w, &h);
  if (!rgb) goto error_file_corrupted;
  free(filebuf);
  filebuf = NULL;

  img->width = w;
  img->height = h;
  img->buf_dsc.channels = 4;
  img->buf_dsc.datatype = TYPE_FLOAT;

  float *buf = (float *)dt_mipmap_cache_alloc(mbuf, img);
  if(!buf) goto error_cache_full;

  dt_imageio_flip_buffers_ui8_to_float(buf, rgb, 0.0f, 255.0f, 3, img->width, img->height, img->width,
                                       img->height, 3 * img->width, 0);
  WebPFree(rgb);

  return DT_IMAGEIO_OK;

error_file_corrupted:
  if (f) fclose(f);
  if (filebuf) free(filebuf);
  if (rgb) WebPFree(rgb);
  return DT_IMAGEIO_FILE_CORRUPTED;
error_cache_full:
  if (f) fclose(f);
  if (filebuf) free(filebuf);
  if (rgb) WebPFree(rgb);
  return DT_IMAGEIO_CACHE_FULL;
}

// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.sh
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
