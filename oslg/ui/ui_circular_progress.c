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

struct circular_progress_data
{
  grub_menu_region_common_t back;
  grub_menu_region_common_t tick;
  grub_menu_region_common_t merge;
  int num_ticks;
  int start_angle;
  int ticks;
  int update;
  int dir;
};

int circular_progress_get_data_size (void)
{
  return sizeof (struct circular_progress_data);
}

#define DEF_TICKS	24
#define MAX_TICKS	100

void circular_progress_init_size (grub_widget_t widget)
{
  struct circular_progress_data *data = widget->data;
  char *p;

  data->tick = get_bitmap (widget, "tick", 0, 0);
  if (! data->tick)
    return;

  data->back = get_bitmap (widget, "background", 0, 0);
  p = grub_widget_get_prop (widget->node, "num_ticks");
  data->num_ticks = (p) ? grub_strtoul (p, 0, 0) : 0;
  if ((data->num_ticks <= 0) || (data->num_ticks > MAX_TICKS))
    data->num_ticks = DEF_TICKS;

  p = grub_widget_get_prop (widget->node, "start_angle");
  data->start_angle = ((p) ? grub_strtol (p, 0, 0) : 0) *
    GRUB_TRIG_ANGLE_MAX / 360;

  p = grub_widget_get_prop (widget->node, "clockwise");
  data->dir = (p) ? -1 : 1;

  if (data->back)
    {
      if (! (widget->node->flags & GRUB_WIDGET_FLAG_FIXED_WIDTH))
	widget->width = data->back->width;

      if (! (widget->node->flags & GRUB_WIDGET_FLAG_FIXED_HEIGHT))
	widget->height = data->back->height;
    }

  data->ticks = -1;
}

void circular_progress_fini_size (grub_widget_t widget)
{
  struct circular_progress_data *data = widget->data;

  grub_menu_region_scale (data->back, widget->width, widget->height);
}

void circular_progress_free (grub_widget_t widget)
{
  struct circular_progress_data *data = widget->data;

  grub_menu_region_free (data->back);
  grub_menu_region_free (data->tick);
}

void circular_progress_draw (grub_widget_t widget,
			grub_menu_region_update_list_t *head,
			int x, int y, int width, int height)
{
  struct circular_progress_data *data = widget->data;

  if (data->update)
    {
      if (data->update < 0)
	grub_menu_region_add_update (head, data->back, widget->org_x,
				     widget->org_y, x, y, width, height);
      grub_menu_region_add_update (head, data->tick, widget->org_x,
			     widget->org_y, x, y, width, height);
      data->update = 0;
    }
}

void circular_progress_set_timeout (grub_widget_t widget, int total, int left)
{
  struct circular_progress_data *data = widget->data;

  if (! data->tick)
    return;

  if (left > 0)
    {
      int ticks = ((total - left) * data->num_ticks) / total;

      if (ticks > data->ticks)
	{
	  int x, y, r, a;

	  x = (widget->width - data->tick->width) / 2;
	  y = (widget->height - data->tick->height) / 2;
	  r = (x > y) ? y : x;

	  a = data->start_angle +
	    (data->dir * ticks * GRUB_TRIG_ANGLE_MAX) / data->num_ticks;
	  x += (grub_cos (a) * r / GRUB_TRIG_FRACTION_SCALE);
	  y -= (grub_sin (a) * r / GRUB_TRIG_FRACTION_SCALE);

	  data->tick->ofs_x = x;
	  data->tick->ofs_y = y;

	  if (data->ticks >= 0)
	    data->update = 1;
	  else
	    data->update = -1;
	  data->ticks = ticks;
	}
    }
}
