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

struct progressbar_data
{
  grub_menu_region_common_t bar;
  grub_menu_region_common_t bg_bar;
  int last_ofs;
  int cur_ofs;
};

int
progressbar_get_data_size (void)
{
  return sizeof (struct progressbar_data);
}

void
progressbar_init_size (grub_widget_t widget)
{
  struct progressbar_data *data = widget->data;
  grub_menu_region_common_t bar;

  data->bar = get_bitmap (widget, "image", 0, &data->bg_bar);
  bar = (data->bar) ? data->bar : data->bg_bar;
  if (bar)
    {
      if (! (widget->node->flags & GRUB_WIDGET_FLAG_FIXED_WIDTH))
	widget->width = bar->width;

      if (! (widget->node->flags & GRUB_WIDGET_FLAG_FIXED_HEIGHT))
	widget->height = bar->height;
    }
  else
    {
      grub_video_color_t color;
      grub_uint32_t fill;
      grub_video_color_t bg_color;
      grub_video_color_t bg_fill;
      char *p;

      p = grub_widget_get_prop (widget->node, "color");
      if (! p)
	p = "";

      fill = bg_fill = 0;
      color = grub_menu_parse_color (p, &fill, &bg_color, &bg_fill);

      data->bar = (grub_menu_region_common_t)
	grub_menu_region_create_rect (0, 0, color, fill);
      if (color != bg_color)
	data->bg_bar = (grub_menu_region_common_t)
	  grub_menu_region_create_rect (0, 0, bg_color, bg_fill);
    }
}

void
progressbar_fini_size (grub_widget_t widget)
{
  struct progressbar_data *data = widget->data;

  grub_menu_region_scale (data->bar, widget->width, widget->height);
  grub_menu_region_scale (data->bg_bar, widget->width, widget->height);
}

void
progressbar_free (grub_widget_t widget)
{
  struct progressbar_data *data = widget->data;

  grub_menu_region_free (data->bar);
  grub_menu_region_free (data->bg_bar);
}

void
progressbar_draw (grub_widget_t widget, grub_menu_region_update_list_t *head,
	    int x, int y, int width, int height)
{
  struct progressbar_data *data = widget->data;
  int x1, y1, w1, h1;

  x1 = data->last_ofs;
  y1 = 0;
  w1 = data->cur_ofs - data->last_ofs;
  h1 = widget->height;
  if (grub_menu_region_check_rect (&x1, &y1, &w1, &h1, x, y, width, height))
    grub_menu_region_add_update (head, data->bar, widget->org_x, widget->org_y,
				 x1, y1, w1, h1);

  if ((! data->last_ofs) && (data->bg_bar))
    {
      x1 = data->cur_ofs;
      y1 = 0;
      w1 = widget->width - x1;
      h1 = widget->height;
      if (grub_menu_region_check_rect (&x1, &y1, &w1, &h1,
         			       x, y, width, height))
	  grub_menu_region_add_update (head, data->bg_bar,
				     widget->org_x, widget->org_y,
				     x1, y1, w1, h1);
    }

  data->last_ofs = data->cur_ofs;
}

void
progressbar_set_timeout (grub_widget_t widget, int total, int left)
{
  struct progressbar_data *data = widget->data;

  if (left > 0)
    data->cur_ofs = (total - left) * widget->width / total;
}
