/*
 *  OSLG  -- Operating System Loader in GRUB 
 *  Copyright (C) 2022 Free Software Foundation, Inc.
 *
 *  OSLG is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OSLG is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with OSLG.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/term.h>
#include <grub/err.h>
#include <grub/widget.h>
#include <grub/lib.h>
#include <grub/time.h>
#include <grub/datetime.h>
#include <grub/bitmap_scale.h>
#include <grub/menu_data.h>
#include <grub/parser.h>
#include <grub/lib.h>
#include <grub/charset.h>
#include <grub/trig.h>
#include "coreui.h"

void adjust_margin (grub_widget_t widget, const char *name, int mode)
{
  char buf[20], *p;
  int len, size;

  grub_strcpy (buf, name);
  len = grub_strlen (buf);
  buf[len++] = '_';

  grub_strcpy (&buf[len], "size");
  p = grub_widget_get_prop (widget->node, buf);
  size = (p) ? grub_menu_parse_size (p, 0, 1) : 0;

  if (mode != MARGIN_HEIGHT)
    {
      int v;

      grub_strcpy (&buf[len], "left");
      p = grub_widget_get_prop (widget->node, buf);
      v = (p) ? grub_menu_parse_size (p, 0, 1) : size;
      if (mode == MARGIN_FINI)
	{
	  widget->inner_x += v;
	  widget->inner_width -= v;
	}
      else
	widget->width += v;

      grub_strcpy (&buf[len], "right");
      p = grub_widget_get_prop (widget->node, buf);
      v = (p) ? grub_menu_parse_size (p, 0, 1) : size;
      if (mode == MARGIN_FINI)
	widget->inner_width -= v;
      else
	widget->width += v;
    }

  if (mode != MARGIN_WIDTH)
    {
      int v;

      grub_strcpy (&buf[len], "top");
      p = grub_widget_get_prop (widget->node, buf);
      v = (p) ? grub_menu_parse_size (p, 0, 0) : size;
      if (mode == MARGIN_FINI)
	{
	  widget->inner_y += v;
	  widget->inner_height -= v;
	}
      else
	widget->height += v;

      grub_strcpy (&buf[len], "bottom");
      p = grub_widget_get_prop (widget->node, buf);
      v = (p) ? grub_menu_parse_size (p, 0, 0) : size;
      if (mode == MARGIN_FINI)
	widget->inner_height -= v;
      else
	widget->height += v;
    }
}

struct screen_data
{
  grub_menu_region_common_t background;
};

int screen_get_data_size (void)
{
  return sizeof (struct screen_data);
}

void screen_init_size (grub_widget_t widget)
{
  struct screen_data *data = widget->data;

  data->background = get_bitmap (widget, "background", 1, 0);
  grub_menu_region_get_screen_size (&widget->width, &widget->height);
  grub_menu_region_scale (data->background, widget->width, widget->height);
}

void screen_fini_size (grub_widget_t widget)
{
  adjust_margin (widget, "margin", MARGIN_FINI);
}

void screen_free (grub_widget_t widget)
{
  struct screen_data *data = widget->data;

  grub_menu_region_free (data->background);
}

void screen_draw (grub_widget_t widget, grub_menu_region_update_list_t *head,
	     int x, int y, int width, int height)
{
  struct screen_data *data = widget->data;

  grub_menu_region_add_update (head, data->background,
			       widget->org_x, widget->org_y,
			       x, y, width, height);
}
