/* iso base media file format box dumper. */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

uint32_t read_size(FILE *in)
{
  uint8_t buf[4];
  size_t read = fread(buf, 1, 4, in);
  if (read != 4) {
    fprintf(stderr, "Error: couldn't read size\n");
    return 0;
  }
  return buf[0] << 24 |
         buf[1] << 16 |
         buf[2] <<  8 |
         buf[3];
}

int read_type(FILE *in, uint8_t *type)
{
  uint8_t buf[4];
  size_t read = fread(buf, 1, 4, in);
  if (read != 4) {
    fprintf(stderr, "Error: couldn't read type\n");
    return -1;
  }
  memcpy(type, buf, 4);
  type[4] = '\0';
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
  version = read_size(in);
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
  while (!feof(in)) {
    /* read size */
    uint32_t size = read_size(in);
    uint8_t type[5];
    if (size < 8) {
      fprintf(stderr, "Error: invalid box size\n");
      return -1;
    }
    /* read type */
    if (read_type(in, type)) {
      fprintf(stderr, "Error: couldn't read box type\n");
      return -2;
    }
    /* report box */
    fprintf(stdout, " '%s' box %u bytes\n", type, size);
    /* dispatch to per-type parsers */
    if (!memcmp(type, "ftyp", 4)) {
      dump_ftyp(in, size);
    }
    else {
      /* skip to the next box */ 
      fseek(in, size - 8, SEEK_CUR);
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
