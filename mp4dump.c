/* iso base media file format box dumper. */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

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

int dump(FILE *in)
{
  while (!feof(in)) {
    uint32_t size = read_size(in);
    uint8_t type[5];
    size_t read;
    if (size < 8) {
      fprintf(stderr, "Error: invalid box size\n");
      return -1;
    }
    read = fread(type, 1, 4, in);
    if (read != 4) {
      fprintf(stderr, "Error: couldn't read type\n");
      return -2;
    }
    type[4] = '\0';
    fprintf(stderr, " '%s' box %u bytes\n", type, size);
    /* skip to the next box */ 
    fseek(in, size - 8, SEEK_CUR);
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
