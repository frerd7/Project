/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009,2010 Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <grub/types.h>
#include <grub/util/misc.h>
#include <grub/misc.h>
#include <grub/handler.h>
#include <grub/i18n.h>
#include <grub/fontformat.h>

#include "progname.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftsynth.h>

void
grub_putchar (int c)
{
  putchar (c);
}

int
grub_getkey (void)
{
  return -1;
}

struct grub_handler_class grub_term_input_class;
struct grub_handler_class grub_term_output_class;

void
grub_refresh (void)
{
  fflush (stdout);
}

char *
grub_env_get (const char *name __attribute__ ((unused)))
{
  return NULL;
}

#define GRUB_FONT_DEFAULT_SIZE		16

#define GRUB_FONT_RANGE_BLOCK		1024

struct grub_glyph_info
{
  struct grub_glyph_info *next;
  grub_uint32_t char_code;
  int width;
  int height;
  int x_ofs;
  int y_ofs;
  int device_width;
  int bitmap_size;
  grub_uint8_t bitmap[0];
};

enum file_formats
{
  PF2,
  ASCII_BITMAPS
};

#define GRUB_FONT_FLAG_BOLD		1
#define GRUB_FONT_FLAG_NOBITMAP		2
#define GRUB_FONT_FLAG_NOHINTING	4
#define GRUB_FONT_FLAG_FORCEHINT	8

struct grub_font_info
{
  char* name;
  int style;
  int desc;
  int asce;
  int size;
  int max_width;
  int max_height;
  int min_y;
  int max_y;
  int flags;
  int num_range;
  int num_point;
  grub_uint32_t *ranges;
  grub_uint32_t *points;
  struct grub_glyph_info *glyph;
};

static struct option options[] =
{
  {"output", required_argument, 0, 'o'},
  {"name", required_argument, 0, 'n'},
  {"index", required_argument, 0, 'I'},
  {"range", required_argument, 0, 'r'},
  {"size", required_argument, 0, 's'},
  {"desc", required_argument, 0, 'd'},
  {"asce", required_argument, 0, 'c'},
  {"bold", no_argument, 0, 'b'},
  {"no-bitmap", no_argument, 0, 0x100},
  {"no-hinting", no_argument, 0, 0x101},
  {"add-ascii", no_argument, 0, 0x103},
  {"add-text", required_argument, 0, 't'},
  {"force-autohint", no_argument, 0, 'a'},
  {"info", no_argument, 0, 'i'},
  {"help", no_argument, 0, 'h'},
  {"version", no_argument, 0, 'V'},
  {"verbose", no_argument, 0, 'v'},
  {"ascii-bitmaps", no_argument, 0, 0x102},
  {0, 0, 0, 0}
};

int font_verbosity;

static void
usage (int status)
{
  if (status)
    fprintf (stderr, "Try `%s --help' for more information.\n", program_name);
  else
    printf ("\
Usage: %s [OPTIONS] FONT_FILES\n\
\nOptions:\n\
  -o, --output=FILE_NAME    set output file name\n\
  --ascii-bitmaps           save only the ASCII bitmaps\n\
  -I, --index=N             set face index\n\
  -r, --range=A-B[,C-D]     set font range\n\
  -n, --name=S              set font family name\n\
  -s, --size=N              set font size\n\
  -d, --desc=N              set font descent\n\
  -c, --asce=N              set font ascent\n\
  -b, --bold                convert to bold font\n\
  -a, --force-autohint      force autohint\n\
  --no-hinting              disable hinting\n\
  --no-bitmap               ignore bitmap strikes when loading\n\
  --add-ascii               add ascii characters\n\
  --add-text,-t FILENAME    add characters from sample file\n\
  -i, --info                    print pf2 font information\n\
  -h, --help                display this message and exit\n\
  -V, --version             print version information and exit\n\
  -v, --verbose             print verbose messages\n\
\n\
Report bugs to <%s>.\n", program_name, PACKAGE_BUGREPORT);

  exit (status);
}

void
add_pixel (grub_uint8_t **data, int *mask, int not_blank)
{
  if (*mask == 0)
    {
      (*data)++;
      **data = 0;
      *mask = 128;
    }

  if (not_blank)
    **data |= *mask;

  *mask >>= 1;
}

void
add_char (struct grub_font_info *font_info, FT_Face face,
	  grub_uint32_t char_code)
{
  struct grub_glyph_info *glyph_info, **p_glyph;
  int width, height;
  grub_uint8_t *data;
  int mask, i, j, bitmap_size;
  FT_GlyphSlot glyph;
  int flag = FT_LOAD_RENDER | FT_LOAD_MONOCHROME;

  if (font_info->flags & GRUB_FONT_FLAG_NOBITMAP)
    flag |= FT_LOAD_NO_BITMAP;

  if (font_info->flags & GRUB_FONT_FLAG_NOHINTING)
    flag |= FT_LOAD_NO_HINTING;
  else if (font_info->flags & GRUB_FONT_FLAG_FORCEHINT)
    flag |= FT_LOAD_FORCE_AUTOHINT;

  if (FT_Load_Char (face, char_code, flag))
    return;

  glyph = face->glyph;

  if (font_info->flags & GRUB_FONT_FLAG_BOLD)
    FT_GlyphSlot_Embolden (glyph);

  p_glyph = &font_info->glyph;
  while ((*p_glyph) && ((*p_glyph)->char_code > char_code))
    {
      p_glyph = &(*p_glyph)->next;
    }

  /* Ignore duplicated glyph.  */
  if ((*p_glyph) && ((*p_glyph)->char_code == char_code))
    return;

  width = glyph->bitmap.width;
  height = glyph->bitmap.rows;

  bitmap_size = ((width * height + 7) / 8);
  glyph_info = xmalloc (sizeof (struct grub_glyph_info) + bitmap_size);
  glyph_info->bitmap_size = bitmap_size;

  glyph_info->next = *p_glyph;
  *p_glyph = glyph_info;

  glyph_info->char_code = char_code;
  glyph_info->width = width;
  glyph_info->height = height;
  glyph_info->x_ofs = glyph->bitmap_left;
  glyph_info->y_ofs = glyph->bitmap_top - height;
  glyph_info->device_width = glyph->metrics.horiAdvance / 64;

  if (width > font_info->max_width)
    font_info->max_width = width;

  if (height > font_info->max_height)
    font_info->max_height = height;

  if (glyph_info->y_ofs < font_info->min_y && glyph_info->y_ofs > -font_info->size)
    font_info->min_y = glyph_info->y_ofs;

  if (glyph_info->y_ofs + height > font_info->max_y)
    font_info->max_y = glyph_info->y_ofs + height;

  mask = 0;
  data = &glyph_info->bitmap[0] - 1;
  for (j = 0; j < height; j++)
    for (i = 0; i < width; i++)
      add_pixel (&data, &mask,
		 glyph->bitmap.buffer[i / 8 + j * glyph->bitmap.pitch] &
		 (1 << (7 - (i & 7))));
}

void
add_font (struct grub_font_info *font_info, FT_Face face)
{
  if ((font_info->num_range) || (font_info->num_point))
    {
      int i;
      grub_uint32_t j;

      for (i = 0; i < font_info->num_range; i++)
	for (j = font_info->ranges[i * 2]; j <= font_info->ranges[i * 2 + 1];
	     j++)
	  add_char (font_info, face, j);

      for (i = 0; i < font_info->num_point; i++)
	add_char (font_info, face, font_info->points[i]);
    }
  else
    {
      grub_uint32_t char_code, glyph_index;

      for (char_code = FT_Get_First_Char (face, &glyph_index);
	   glyph_index;
	   char_code = FT_Get_Next_Char (face, char_code, &glyph_index))
	add_char (font_info, face, char_code);
    }
}

void
write_string_section (char *name, char *str, int* offset, FILE* file)
{
  grub_uint32_t leng, leng_be32;

  leng = strlen (str) + 1;
  leng_be32 = grub_cpu_to_be32 (leng);

  grub_util_write_image (name, 4, file);
  grub_util_write_image ((char *) &leng_be32, 4, file);
  grub_util_write_image (str, leng, file);

  *offset += 8 + leng;
}

void
write_be16_section (char *name, grub_uint16_t data, int* offset, FILE* file)
{
  grub_uint32_t leng;

  leng = grub_cpu_to_be32 (2);
  data = grub_cpu_to_be16 (data);
  grub_util_write_image (name, 4, file);
  grub_util_write_image ((char *) &leng, 4, file);
  grub_util_write_image ((char *) &data, 2, file);

  *offset += 10;
}

void
print_glyphs (struct grub_font_info *font_info)
{
  int num;
  struct grub_glyph_info *glyph;
  char line[512];

  for (glyph = font_info->glyph, num = 0; glyph; glyph = glyph->next, num++)
    {
      int x, y, xmax, xmin, ymax, ymin;
      grub_uint8_t *bitmap, mask;

      printf ("\nGlyph #%d, U+%04x\n", num, glyph->char_code);
      printf ("Width %d, Height %d, X offset %d, Y offset %d,"
	      "Device width %d\n",
	      glyph->width, glyph->height, glyph->x_ofs, glyph->y_ofs,
	      glyph->device_width);

      xmax = glyph->x_ofs + glyph->width;
      if (xmax < glyph->device_width)
	xmax = glyph->device_width;

      xmin = glyph->x_ofs;
      if (xmin > 0)
	xmin = 0;

      ymax = glyph->y_ofs + glyph->height;
      if (ymax < font_info->asce)
	ymax = font_info->asce;

      ymin = glyph->y_ofs;
      if (ymin > - font_info->desc)
	ymin = - font_info->desc;

      bitmap = glyph->bitmap;
      mask = 0x80;
      for (y = ymax - 1; y >= ymin; y--)
	{
	  int line_pos;

	  line_pos = 0;
	  for (x = xmin; x < xmax; x++)
	    {
	      if ((x >= glyph->x_ofs) &&
		  (x < glyph->x_ofs + glyph->width) &&
		  (y >= glyph->y_ofs) &&
		  (y < glyph->y_ofs + glyph->height))
		{
		  line[line_pos++] = (*bitmap & mask) ? '#' : '_';
		  mask >>= 1;
		  if (mask == 0)
		    {
		      mask = 0x80;
		      bitmap++;
		    }
		}
	      else if ((x >= 0) &&
		       (x < glyph->device_width) &&
		       (y >= - font_info->desc) &&
		       (y < font_info->asce))
		{
		  line[line_pos++] = ((x == 0) || (y == 0)) ? '+' : '.';
		}
	      else
		line[line_pos++] = '*';
	    }
	  line[line_pos] = 0;
	  printf ("%s\n", line);
	}
    }
}

void
write_font_ascii_bitmap (struct grub_font_info *font_info, char *output_file)
{
  FILE *file;
  struct grub_glyph_info *glyph;
  int num;

  file = fopen (output_file, "wb");
  if (! file)
    grub_util_error ("Can\'t write to file %s.", output_file);

  int correct_size;
  for (glyph = font_info->glyph, num = 0; glyph; glyph = glyph->next, num++)
    {
      correct_size = 1;
      if (glyph->width != 8 || glyph->height != 16)
      {
        /* printf ("Width or height from glyph U+%04x not supported, skipping.\n", glyph->char_code);  */
	correct_size = 0;
      }
      int row;
      for (row = 0; row < glyph->height; row++)
        {
	  if (correct_size)
	    fwrite (&glyph->bitmap[row], sizeof(glyph->bitmap[row]), 1, file);
	  else
	    fwrite (&correct_size, 1, 1, file);
        }
    }
    fclose (file);
}

void
write_font_pf2 (struct grub_font_info *font_info, char *output_file)
{
  FILE *file;
  grub_uint32_t leng, data;
  char style_name[20], *font_name;
  struct grub_glyph_info *cur, *pre;
  int num, offset;

  file = fopen (output_file, "wb");
  if (! file)
    grub_util_error ("can\'t write to file %s.", output_file);

  offset = 0;

  leng = grub_cpu_to_be32 (4);
  grub_util_write_image (FONT_FORMAT_SECTION_NAMES_FILE,
  			 sizeof(FONT_FORMAT_SECTION_NAMES_FILE) - 1, file);
  grub_util_write_image ((char *) &leng, 4, file);
  grub_util_write_image (FONT_FORMAT_PFF2_MAGIC, 4, file);
  offset += 12;

  if (! font_info->name)
    font_info->name = "Unknown";

  if (font_info->flags & GRUB_FONT_FLAG_BOLD)
    font_info->style |= FT_STYLE_FLAG_BOLD;

  style_name[0] = 0;
  if (font_info->style & FT_STYLE_FLAG_BOLD)
    strcpy (style_name, " Bold");

  if (font_info->style & FT_STYLE_FLAG_ITALIC)
    strcat (style_name, " Italic");

  if (! style_name[0])
    strcpy (style_name, " Regular");

  font_name = xasprintf ("%s %s %d", font_info->name, &style_name[1],
			 font_info->size);

  write_string_section (FONT_FORMAT_SECTION_NAMES_FONT_NAME,
  			font_name, &offset, file);
  write_string_section (FONT_FORMAT_SECTION_NAMES_FAMILY,
  			font_info->name, &offset, file);
  write_string_section (FONT_FORMAT_SECTION_NAMES_WEIGHT,
			(font_info->style & FT_STYLE_FLAG_BOLD) ?
			"bold" : "normal",
			&offset, file);
  write_string_section (FONT_FORMAT_SECTION_NAMES_SLAN,
			(font_info->style & FT_STYLE_FLAG_ITALIC) ?
			"italic" : "normal",
			&offset, file);

  write_be16_section (FONT_FORMAT_SECTION_NAMES_POINT_SIZE,
  		      font_info->size, &offset, file);
  write_be16_section (FONT_FORMAT_SECTION_NAMES_MAX_CHAR_WIDTH,
  		      font_info->max_width, &offset, file);
  write_be16_section (FONT_FORMAT_SECTION_NAMES_MAX_CHAR_HEIGHT,
  		      font_info->max_height, &offset, file);

  if (! font_info->desc)
    {
      if (font_info->min_y >= 0)
	font_info->desc = 1;
      else
	font_info->desc = - font_info->min_y;
    }

  if (! font_info->asce)
    {
      if (font_info->max_y <= 0)
	font_info->asce = 1;
      else
	font_info->asce = font_info->max_y;
    }

  write_be16_section (FONT_FORMAT_SECTION_NAMES_ASCENT,
  		      font_info->asce, &offset, file);
  write_be16_section (FONT_FORMAT_SECTION_NAMES_DESCENT,
  		      font_info->desc, &offset, file);

  if (font_verbosity > 0)
    {
      printf ("Font name: %s\n", font_name);
      printf ("Max width: %d\n", font_info->max_width);
      printf ("Max height: %d\n", font_info->max_height);
      printf ("Font ascent: %d\n", font_info->asce);
      printf ("Font descent: %d\n", font_info->desc);
    }

  num = 0;
  pre = 0;
  cur = font_info->glyph;
  while (cur)
    {
      struct grub_glyph_info *nxt;

      nxt = cur->next;
      cur->next = pre;
      pre = cur;
      cur = nxt;
      num++;
    }

  font_info->glyph = pre;

  if (font_verbosity > 0)
    printf ("Number of glyph: %d\n", num);

  leng = grub_cpu_to_be32 (num * 9);
  grub_util_write_image (FONT_FORMAT_SECTION_NAMES_CHAR_INDEX,
  			 sizeof(FONT_FORMAT_SECTION_NAMES_CHAR_INDEX) - 1,
			 file);
  grub_util_write_image ((char *) &leng, 4, file);
  offset += 8 + num * 9 + 8;

  for (cur = font_info->glyph; cur; cur = cur->next)
    {
      data = grub_cpu_to_be32 (cur->char_code);
      grub_util_write_image ((char *) &data, 4, file);
      data = 0;
      grub_util_write_image ((char *) &data, 1, file);
      data = grub_cpu_to_be32 (offset);
      grub_util_write_image ((char *) &data, 4, file);
      offset += 10 + cur->bitmap_size;
    }

  leng = 0xffffffff;
  grub_util_write_image (FONT_FORMAT_SECTION_NAMES_DATA,
  			 sizeof(FONT_FORMAT_SECTION_NAMES_DATA) - 1, file);
  grub_util_write_image ((char *) &leng, 4, file);

  for (cur = font_info->glyph; cur; cur = cur->next)
    {
      data = grub_cpu_to_be16 (cur->width);
      grub_util_write_image ((char *) &data, 2, file);
      data = grub_cpu_to_be16 (cur->height);
      grub_util_write_image ((char *) &data, 2, file);
      data = grub_cpu_to_be16 (cur->x_ofs);
      grub_util_write_image ((char *) &data, 2, file);
      data = grub_cpu_to_be16 (cur->y_ofs);
      grub_util_write_image ((char *) &data, 2, file);
      data = grub_cpu_to_be16 (cur->device_width);
      grub_util_write_image ((char *) &data, 2, file);
      grub_util_write_image ((char *) &cur->bitmap[0], cur->bitmap_size, file);
    }

  fclose (file);
}

static void
add_point (struct grub_font_info *font_info, grub_uint32_t a)
{
  int i;

  for (i = 0; i < font_info->num_point; i++)
    if (font_info->points[i] == a)
      return;

  if ((font_info->num_point & (GRUB_FONT_RANGE_BLOCK - 1)) == 0)
    font_info->points = xrealloc (font_info->points,
				  (font_info->num_point +
				   GRUB_FONT_RANGE_BLOCK) *
				  sizeof (grub_uint32_t));
  font_info->points[font_info->num_point++] = a;
}

static void
add_range (struct grub_font_info *font_info, grub_uint32_t a,
	   grub_uint32_t b)
{
  if (a == b)
    add_point (font_info, a);
  else if (a < b)
    {
      if ((font_info->num_range & (GRUB_FONT_RANGE_BLOCK - 1)) == 0)
	font_info->ranges = xrealloc (font_info->ranges,
				     (font_info->num_range +
				      GRUB_FONT_RANGE_BLOCK) *
				      sizeof (grub_uint32_t) * 2);
      font_info->ranges[font_info->num_range * 2] = a;
      font_info->ranges[font_info->num_range * 2 + 1] = b;
      font_info->num_range++;
    }
}

static void
add_file (struct grub_font_info *font_info, const char *name)
{
  grub_uint8_t *data;
  const grub_uint8_t *ptr;
  size_t size;
  grub_uint32_t code;

  size = grub_util_get_image_size (name);
  data = (grub_uint8_t *) grub_util_read_image (name);
  ptr = (const grub_uint8_t *) data;
  while ((ptr < data + size) &&
	 (grub_utf8_to_ucs4 (&code, 1, ptr, -1, &ptr) > 0))
    {
      if ((code != 0) && (code != 0xfeff) &&
	  (code != '\r') && (code != '\n'))
	add_point (font_info, code);
    }

  free (data);
}

static char *
read_section (char **str, int *value, off_t *ofs, FILE* file)
{
  static char section_name[8];
  char *p;
  int len;

  *str = NULL;
  *value = 0;
  grub_util_read_at (section_name, 8, *ofs, file);
  len = grub_be_to_cpu32 (*((grub_uint32_t *) (section_name + 4)));
  *ofs += 8;
  p = section_name;
  if (len == 2)
    {
      grub_util_read_at (section_name + 4, 2, *ofs, file);
      *value = grub_be_to_cpu16 (*((grub_uint16_t *) (section_name + 4)));
    }
  else
    {
      if (memcmp (section_name, "CHIX", 4))
	{
	  *str = xmalloc (len);
	  grub_util_read_at (*str, len, *ofs, file);
	}
      else
	p = NULL;
    }

  *ofs += len;
  section_name[4] = 0;
  return p;
}

static void
print_info (const char *filename)
{
  FILE *file;
  char buf[12];
  char *name, *str;
  int value;
  off_t ofs;

  file = fopen (filename, "rb");
  if (! file)
    grub_util_error ("can\'t read from file %s", filename);

  grub_util_read_at (buf, 12, 0, file);
  if ((memcmp (buf, "FILE", 4)) ||
      (grub_be_to_cpu32 (*((grub_uint32_t *) (buf + 4))) != 4) ||
      (memcmp (buf + 8, "PFF2", 4)))
    grub_util_error ("invalid format");

  if (font_verbosity > 0)
    printf ("%s:\n", filename);

  ofs = 12;
  while ((name = read_section (&str, &value, &ofs, file)) != NULL)
    {
      if (font_verbosity > 0)
	{
	  if (str)
	    printf ("%s: %s\n", name, str);
	  else
	    printf ("%s: %d\n", name, value);
	}
      else if (! strcmp (name, "NAME"))
	printf ("%s: %s\n", str, filename);
      free (str);
    }

  fclose (file);
}

static grub_uint32_t box_chars[] =
  {
    0x250F, 0x2501, 0x2513, 0x2503, 0x2503, 0x2517, 0x2501, 0x251B,
    0x2554, 0x2550, 0x2557, 0x2551, 0x2551, 0x255A, 0x2550, 0x255D, 0
  };

int
main (int argc, char *argv[])
{
  struct grub_font_info font_info;
  FT_Library ft_lib;
  int font_index = 0;
  int font_size = 0;
  char *output_file = NULL;
  int info_mode = 0;
  enum file_formats file_format = PF2;

  memset (&font_info, 0, sizeof (font_info));

  set_program_name (argv[0]);

  grub_util_init_nls ();

  /* Check for options.  */
  while (1)
    {
      int c = getopt_long (argc, argv, "bao:n:I:s:d:r:t:ihVv", options, 0);

      if (c == -1)
	break;
      else
	switch (c)
	  {
	  case 'b':
	    font_info.flags |= GRUB_FONT_FLAG_BOLD;
	    break;

	  case 0x100:
	    font_info.flags |= GRUB_FONT_FLAG_NOBITMAP;
	    break;

	  case 0x101:
	    font_info.flags |= GRUB_FONT_FLAG_NOHINTING;
	    break;

	  case 0x103:
	    {
	      grub_uint32_t *c;

	      add_range (&font_info, 32, 126);
	      for (c = box_chars; *c; c++)
		add_point (&font_info, *c);
	      break;
	    }

	  case 'a':
	    font_info.flags |= GRUB_FONT_FLAG_FORCEHINT;
	    break;

	  case 'o':
	    output_file = optarg;
	    break;

	  case 'n':
	    font_info.name = optarg;
	    break;

	  case 'I':
	    font_index = strtoul (optarg, NULL, 0);
	    break;

	  case 's':
	    font_size = strtoul (optarg, NULL, 0);
	    break;

	  case 'r':
	    {
	      char *p = optarg;

	      while (1)
		{
		  grub_uint32_t a, b;

		  a = strtoul (p, &p, 0);
		  if (*p == '-')
		    {
		      b = strtoul (p + 1, &p, 0);
		      add_range (&font_info, a, b);
		    }
		  else
		    add_point (&font_info, a);

		  if (*p)
		    {
		      if (*p != ',')
			grub_util_error ("invalid font range");
		      else
			p++;
		    }
		  else
		    break;
		}
	      break;
	    }

	  case 't':
	    add_file (&font_info, optarg);
	    break;

	  case 'i':
	    info_mode = 1;
	    break;

	  case 'd':
	    font_info.desc = strtoul (optarg, NULL, 0);
	    break;

	  case 'e':
	    font_info.asce = strtoul (optarg, NULL, 0);
	    break;

	  case 'h':
	    usage (0);
	    break;

	  case 'V':
	    printf ("%s (%s) %s\n", program_name, PACKAGE_NAME, PACKAGE_VERSION);
	    return 0;

	  case 'v':
	    font_verbosity++;
	    break;

	  case 0x102:
	     file_format = ASCII_BITMAPS;
	     break;

	  default:
	    usage (1);
	    break;
	  }
    }

  if (info_mode)
    {
      for (; optind < argc; optind++)
	print_info (argv[optind]);

      return 0;
    }

  if (file_format == ASCII_BITMAPS && font_info.num_range > 0)
    {
      grub_util_error ("Option --ascii-bitmaps doesn't accept ranges (use ASCII).");
      return 1;
    }
  else if (file_format == ASCII_BITMAPS)
    {
      font_info.ranges = xrealloc (font_info.ranges,
				   GRUB_FONT_RANGE_BLOCK *
				   sizeof (grub_uint32_t) * 2);

      font_info.ranges[0] = (grub_uint32_t) 0x00;
      font_info.ranges[1] = (grub_uint32_t) 0x7f;
      font_info.num_range = 1;
    }

  if (! output_file)
    grub_util_error ("no output file is specified");

  if (FT_Init_FreeType (&ft_lib))
    grub_util_error ("FT_Init_FreeType fails");

  for (; optind < argc; optind++)
    {
      FT_Face ft_face;
      int size;

      if (FT_New_Face (ft_lib, argv[optind], font_index, &ft_face))
	{
	  grub_util_info ("can't open file %s, index %d", argv[optind],
			  font_index);
	  continue;
	}

      if ((! font_info.name) && (ft_face->family_name))
	font_info.name = xstrdup (ft_face->family_name);

      size = font_size;
      if (! size)
	{
	  if ((ft_face->face_flags & FT_FACE_FLAG_SCALABLE) ||
	      (! ft_face->num_fixed_sizes))
	    size = GRUB_FONT_DEFAULT_SIZE;
	  else
	    size = ft_face->available_sizes[0].height;
	}

      font_info.style = ft_face->style_flags;
      font_info.size = size;

      FT_Set_Pixel_Sizes (ft_face, size, size);
      add_font (&font_info, ft_face);
      FT_Done_Face (ft_face);
    }

  FT_Done_FreeType (ft_lib);

  if (file_format == PF2)
    write_font_pf2 (&font_info, output_file);
  else if (file_format == ASCII_BITMAPS)
    write_font_ascii_bitmap (&font_info, output_file);

  if (font_verbosity > 1)
    print_glyphs (&font_info);

  return 0;
}
