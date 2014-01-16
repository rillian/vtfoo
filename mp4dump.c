/* iso base media file format box dumper. */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct {
  uint32_t size;
  uint8_t type[5];
} box;

int read_size(FILE *in, uint32_t *size)
{
  uint8_t buf[4];
  size_t read = fread(buf, 1, 4, in);
  if (read != 4) {
    /* TODO: handle extended size */
    fprintf(stderr, "Error: couldn't read size\n");
    return -1;
  }
  *size = buf[0] << 24 |
          buf[1] << 16 |
          buf[2] <<  8 |
          buf[3];
  return 0;
}

int read_type(FILE *in, uint8_t *type)
{
  uint8_t buf[4];
  size_t read = fread(buf, 1, 4, in);
  if (read != 4) {
    fprintf(stderr, "Error: couldn't read type\n");
    return -1;
  }
  /* TODO: handle uuid types */
  memcpy(type, buf, 4);
  type[4] = '\0';
  return 0;
}

int read_box(FILE *in, box *box)
{
  int ret;

  if (!box)
    return -8;

  ret = read_size(in, &box->size);
  if (ret) return ret;
  ret = read_type(in, box->type);
  if (ret) return ret;

  return 0;
}

/* dump an 'ftyp' box, assuming the header is already read */
int dump_ftyp(FILE *in, uint32_t size)
{
  uint8_t brand[5];
  uint32_t version;
  int brands;

  if (size < 8)
    return -8;
  brands = (size - 8) / 4;
  if (read_type(in, brand)) {
    fprintf(stderr, "Error: couldn't read brand\n");
    return -1;
  }
  if (read_size(in, &version))
      return -2;
  fprintf(stdout, "   %s version %u\n", brand, version);
  if (brands > 0) {
    fprintf(stdout, "   compatible with:");
    for (int i = 0; i < brands; i++) {
      read_type(in, brand);
      brand[4] = '\0';
      fprintf(stdout, " %s", brand);
    }
    fprintf(stdout, "\n");
  }
  return 0;
}

/* dump a file */
int dump(FILE *in)
{
  int ret;
  while (!feof(in)) {
    box box;
    ret = read_box(in, &box);
    if (ret) {
      fprintf(stderr, "Error: couldn't read box header\n");
      return ret;
    }
    if (box.size < 8) {
      fprintf(stderr, "Error: invalid box size\n");
      return -1;
    }
    /* report box */
    fprintf(stdout, " '%s' box %u bytes\n", box.type, box.size);
    /* dispatch to per-type parsers */
    if (!memcmp(box.type, "ftyp", 4)) {
      dump_ftyp(in, box.size);
    }
    else {
      /* skip to the next box */ 
      fseek(in, box.size - 8, SEEK_CUR);
    }
  }
  return 0;
}

int main(int argc, char *argv[])
{
  for (int i = 1; i < argc; i++) {
    char *fn = argv[i];
    FILE *in = fopen(fn, "rb");
    int ret;
    /* See if we can open the file. */
    if (!in) {
      fprintf(stderr, "Couldn't open '%s'\n", fn);
      return i;
    }
    fprintf(stdout, "--- dump of %s ---\n", argv[i]);
    ret = dump(in);
    fclose(in);
    /* Report if we failed to dump it all. */
    if (ret) {
      fprintf(stderr, "Error: parse failure\n");
      return ret;
    }
  }
  /* Success. */
  return 0;
}
