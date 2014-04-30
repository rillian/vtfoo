/* iso base media file format box dumper. */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* General box structure */
typedef struct {
  uint32_t size;
  uint8_t type[5];
  int level;
} box;

/* Just the 'fullbox' header */
typedef struct {
  uint8_t version;
  uint32_t flags;
} fullbox;

uint16_t read_u16(FILE *in)
{
  uint8_t buf[2];
  fread(buf, 1, sizeof(buf), in);
  return buf[0] << 8 | buf[1];
}

uint32_t read_u32(FILE *in)
{
  uint8_t buf[4];
  fread(buf, 1, sizeof(buf), in);
  return buf[0] << 24 |
         buf[1] << 16 |
         buf[2] <<  8 |
         buf[3];
}

uint64_t read_u64(FILE *in)
{
  uint8_t buf[8];
  fread(buf, 1, sizeof(buf), in);
  return (uint64_t)buf[0] << 56 |
         (uint64_t)buf[1] << 48 |
         (uint64_t)buf[2] << 40 |
         (uint64_t)buf[3] << 32 |
         (uint64_t)buf[4] << 24 |
         (uint64_t)buf[5] << 16 |
         (uint64_t)buf[6] <<  8 |
         (uint64_t)buf[7];
}

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

int read_fullbox(FILE *in, box *box, fullbox *header)
{
  uint8_t version;
  uint32_t flags;

  if (box->size < 16) {
    fprintf(stderr, "Error: '%s' too short for fullbox header.\n", box->type);
    return -1;
  }
  fread(&flags, 4, 1, in);
  version = flags >> 24;
  flags = flags & 0x0fff;
  if (version > 1) {
    fprintf(stderr, "Warning: '%s' version > 1.\n", box->type);
    return -1;
  }
 
  header->version = version;
  header->flags = flags;

  return 0;
}

int read_box(FILE *in, box *box)
{
  int ret;

  if (!box)
    return -8;

  box->level = 0;
  ret = read_size(in, &box->size);
  if (ret) return ret;
  ret = read_type(in, box->type);
  if (ret) return ret;
  if (box->size < 8) {
    fprintf(stderr, "Warning: unhandled large box or invalid size.\n");
  }

  return 0;
}

/* forward protoype */
int dump_container(FILE *in, box *parent);

/* dump box header */
static void _dump_box_helper(box *box)
{
  for (int i = 0; i < box->level; i++) {
    fprintf(stdout, " ");
  }
  fprintf(stdout, " '%s' box %u bytes", box->type, box->size);
}
void dump_box(box *box)
{
  _dump_box_helper(box);
  fprintf(stdout, "\n");
}

/* dump fullbox header */
void dump_fullbox(box *box, fullbox *header)
{
  _dump_box_helper(box);
  fprintf(stdout, "\tVersion %d flags 0x%06x\n",
      header->version, header->flags);
}

/* dump an 'ftyp' box, assuming the header is already read */
int dump_ftyp(FILE *in, uint32_t size)
{
  uint8_t brand[5];
  uint32_t version;
  int brands;

  if (size < 16)
    return -8;
  brands = (size - 16) / 4;
  if (read_type(in, brand)) {
    fprintf(stderr, "Error: couldn't read brand\n");
    return -1;
  }
  if (read_size(in, &version))
      return -2;
  fprintf(stdout, "   %s version %u\n", brand, version);
  if (brands > 0) {
    fprintf(stdout, "   compatible with:");
  }
  for (int i = 0; i < brands; i++) {
    read_type(in, brand);
    brand[4] = '\0';
    if (brand[0] == '\0') {
      fprintf(stdout, " ''");
    } else {
      fprintf(stdout, " %s", brand);
    }
  }
  fprintf(stdout, "\n");
  return 0;
}

/* dump media header */
int dump_mvhd(FILE *in, box *mhdr)
{
  fullbox header;
  double duration_s;

  if (mhdr->size < 8 + 100) {
    fprintf(stderr, "Error: 'mvhd' too short.\n");
    return -1;
  }
  read_fullbox(in, mhdr, &header);
  dump_fullbox(mhdr, &header);
  if (header.version > 1) {
    fprintf(stderr, "Error: unknown 'mvhd' version.\n");
    return -1;
  }
  if (header.version == 1) {
    if (mhdr->size < 8 + 112) {
      fprintf(stderr, "Error: 'mvhd' too short.\n");
      return -1;
    }
    uint64_t creation_time = read_u64(in);
    uint64_t modification_time = read_u64(in);
    uint32_t timescale = read_u32(in);
    uint64_t duration = read_u64(in);
    duration_s = (double)duration/timescale;
    fprintf(stdout, "    Created  %llu\n", (unsigned long long)creation_time);
    fprintf(stdout, "    Modified %llu\n", (unsigned long long)modification_time);
    fprintf(stdout, "    Timescale %lu\n", (unsigned long)timescale);
    fprintf(stdout, "    Duration %llu\n", (unsigned long long)duration);
  } else {
    // version == 0 uses 32-bit time fields.
    uint32_t creation_time = read_u32(in);
    uint32_t modification_time = read_u32(in);
    uint32_t timescale = read_u32(in);
    uint32_t duration = read_u32(in);
    duration_s = (double)duration/timescale;
    fprintf(stdout, "    Created  %lu\n", (unsigned long)creation_time);
    fprintf(stdout, "    Modified %lu\n", (unsigned long)modification_time);
    fprintf(stdout, "    Timescale %lu\n", (unsigned long)timescale);
    fprintf(stdout, "    Duration %lu\n", (unsigned long)duration);
  }
  fprintf(stdout, "    Duration %lf seconds\n", duration_s);
  uint32_t rate = read_u32(in);
  uint16_t volume = read_u16(in);
  fprintf(stdout, "    rate %lf\n", (double)rate/0x00010000);
  fprintf(stdout, "    volume %lf\n", (double)volume/0x0100);
  uint16_t reserved0 = read_u16(in);
  uint32_t reserved1 = read_u32(in);
  uint32_t reserved2 = read_u32(in);
  if (reserved0 || reserved1 || reserved2) {
    fprintf(stderr, "Warning: non-zero reserved fields.\n");
  }
  fprintf(stdout, "    Transformation matrix:\n");
  for (int i = 0; i < 3; i++) {
    uint32_t a = read_u32(in);
    uint32_t b = read_u32(in);
    uint32_t c = read_u32(in);
    // Just guessing at the radix here.
    fprintf(stdout,"      %lf %lf %lf\n",
        (double)a/0x00010000, (double)b/0x00010000, (double)c/0x40000000);
  }
  for (int i = 0; i < 6; i++) {
    uint32_t pre_defined = read_u32(in);
    if (pre_defined) {
      fprintf(stderr, "Warning: non-zero mvhd pre_defined field.\n");
    }
  }
  uint32_t next_track_ID = read_u32(in);
  fprintf(stderr, "    next track id %u\n", (unsigned)next_track_ID);
  return 0;
}

/* dump movie extents header data */
int dump_mehd(FILE *in, box *mehd)
{
  fullbox header;

  if (mehd->size < 16) {
    fprintf(stderr, "Error: 'mehd' too short.\n");
    return -1;
  }
  read_fullbox(in, mehd, &header);
  dump_fullbox(mehd, &header);
  if (header.version == 1) {
    if (mehd->size < 12 + 8) {
      fprintf(stderr, "Error: 'mehd' too short.\n");
      return -1;
    }
    uint64_t fragment_duration = read_u64(in);
    fprintf(stdout, "     fragment duration: %llu\n",
        (unsigned long long)fragment_duration);
  } else{
    uint32_t fragment_duration = read_u32(in);
    fprintf(stdout, "     fragment duration: %lu\n",
        (unsigned long)fragment_duration);
  }
  return 0;
}

/* dump movie fragment header box */
int dump_mfhd(FILE *in, box *mfhd)
{
  fullbox header;

  if (mfhd->size < 16) {
    fprintf(stderr, "Error: 'mfhd' too short.\n");
    return -1;
  }
  read_fullbox(in, mfhd, &header);
  dump_fullbox(mfhd, &header);
  if (header.version > 0) {
    fprintf(stderr, "Error: unknown 'mfhd' version.\n");
    return -1;
  }
  if (header.flags != 0) {
    fprintf(stderr, "Warning: non-zero 'mfhd' flags.\n");
  }

  /* Only the sequence number */
  uint32_t sequence_number = read_u32(in);
  fprintf(stderr, "     sequence number %lu\n",
      (unsigned long)sequence_number);

  return 0;
}

/* dump media data */
int dump_mdat(FILE *in, box *mdat)
{
  /* HACK: just copy some to a file for testing */
  const size_t bufsize = 4096*1024;
  uint8_t *buf = malloc(bufsize);
  size_t read = 0;;
  FILE *out;
  if (!buf) {
    fprintf(stderr, "Error: couldn't allocate mdat copy buffer\n");
    return -8;
  }
  out = fopen("mdat", "wb");
  if (!out) {
    fprintf(stderr, "Error: couldn't open output file\n");
    free(buf);
    return -1;
  }
  while (read < mdat->size && !feof(in)) {
    size_t bytes = mdat->size - read;
    if (bytes > bufsize) bytes = bufsize;
    bytes = fread(buf, 1, bytes, in);
    if (bytes < 1) {
      fprintf(stderr, "Error: couldn't read mdat data\n");
      break;
    }
    fwrite(buf, 1, bytes, out);
    read += bytes;
  }
  fclose(in);
  free(buf);
  return 0;
}

/* dispatch by box type, with recursion */
int dispatch(FILE *in, box *box)
{
  if (!memcmp(box->type, "ftyp", 4))
    return dump_ftyp(in, box->size);
  if (!memcmp(box->type, "moov", 4))
    return dump_container(in, box);
  if (!memcmp(box->type, "mvhd", 4))
    return dump_mvhd(in, box);
  if (!memcmp(box->type, "mvex", 4))
    return dump_container(in, box);
  if (!memcmp(box->type, "mehd", 4))
    return dump_mehd(in, box);
  if (!memcmp(box->type, "mdia", 4))
    return dump_container(in, box);
  if (!memcmp(box->type, "minf", 4))
    return dump_container(in, box);
  if (!memcmp(box->type, "stbl", 4))
    return dump_container(in, box);
  if (!memcmp(box->type, "trak", 4))
    return dump_container(in, box);
  if (!memcmp(box->type, "moof", 4))
    return dump_container(in, box);
  if (!memcmp(box->type, "mfhd", 4))
    return dump_mfhd(in, box);
  if (!memcmp(box->type, "mdat", 4))
    //return dump_mdat(in, box);
    dump_box(box);
  /* Unhandled: skip to the next box */
  fseek(in, box->size - 8, SEEK_CUR);
  return 0;
}

/* dump a container box */
int dump_container(FILE *in, box *parent)
{
  box box;
  int ret;
  size_t read = 0;
  while (read < parent->size - 8) {
    ret = read_box(in, &box);
    if (ret) {
      fprintf(stderr, "Error: couldn't read contained box\n");
      return ret;
    }
    box.level = parent->level + 1;
    /* HACK: reset some box level until we have extent-based ends. */
    if (!memcmp(box.type, "moof", 4)) {
      box.level = 0;
    }
    if (!memcmp(box.type, "trak", 4)) {
      box.level = 1;
    }
    dump_box(&box);
    ret = dispatch(in, &box);
    if (ret) {
      fprintf(stderr, "Error: couldn't parse contained box\n");
      return ret;
    }
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
    dump_box(&box);
    /* dispatch to per-type parsers */
    dispatch(in, &box);
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
