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

struct image_data
{
  grub_menu_region_common_t image;
  grub_menu_region_common_t image_selected;
};

int image_get_data_size (void)
{
  return sizeof (struct image_data);
}

void image_init_size (grub_widget_t widget)
{
  struct image_data *data = widget->data;

  data->image = get_bitmap (widget, "image", 0, &data->image_selected);
  if (! data->image)
    return;

  if (data->image_selected)
    widget->node->flags |= GRUB_WIDGET_FLAG_DYNAMIC;
  else
    widget->node->flags &= ~GRUB_WIDGET_FLAG_DYNAMIC;

  if (! (widget->node->flags & GRUB_WIDGET_FLAG_FIXED_WIDTH))
    widget->width = data->image->width;

  if (! (widget->node->flags & GRUB_WIDGET_FLAG_FIXED_HEIGHT))
    widget->height = data->image->height;
}

void image_fini_size (grub_widget_t widget)
{
  struct image_data *data = widget->data;

  grub_menu_region_scale (data->image, widget->width, widget->height);
  grub_menu_region_scale (data->image_selected, widget->width, widget->height);
}

void image_free (grub_widget_t widget)
{
  struct image_data *data = widget->data;

  grub_menu_region_free (data->image);
  grub_menu_region_free (data->image_selected);
}

void image_draw (grub_widget_t widget, grub_menu_region_update_list_t *head,
	    int x, int y, int width, int height)
{
  struct image_data *data = widget->data;
  grub_menu_region_common_t image;

  image = ((widget->node->flags & GRUB_WIDGET_FLAG_SELECTED)
	   && data->image_selected) ? data->image_selected : data->image;

  grub_menu_region_add_update (head, image, widget->org_x, widget->org_y,
			       x, y, width, height);
}
