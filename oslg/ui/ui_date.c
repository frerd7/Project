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

struct text_data
{
  grub_menu_region_text_t text;
  grub_video_color_t color;
  grub_video_color_t color_selected;
};

int
date_get_data_size (void)
{
  return sizeof (struct text_data);
}

static int date_time(grub_widget_t widget, grub_font_t font);

static int date_time(grub_widget_t widget, grub_font_t font)
{
  struct text_data *data = widget->data;

  char buffer[120];
  struct grub_datetime datetime;
      if (grub_get_datetime (&datetime))
        return grub_errno;
    
   grub_snprintf(buffer,sizeof(buffer),"%s %02d/%02d/%02d %02d:%02d",grub_get_weekday_name (&datetime), datetime.day, datetime.month,datetime.year,datetime.hour, datetime.minute);  
  
  data->text = grub_menu_region_create_text (font, 0, buffer);

return sizeof(datetime);
}

void
date_init_size (grub_widget_t widget)
{
  struct text_data *data = widget->data;
  grub_font_t font;
  char *p;

  widget->node->flags |= GRUB_WIDGET_FLAG_TRANSPARENT;

  font = grub_menu_region_get_font ("Unifont Regular 16");

  p = grub_widget_get_prop (widget->node, "color");
  if (p)
    data->color = grub_menu_parse_color (p, 0, &data->color_selected, 0);

  if (data->color != data->color_selected)
    widget->node->flags |= GRUB_WIDGET_FLAG_DYNAMIC;
  else
    widget->node->flags &= ~GRUB_WIDGET_FLAG_DYNAMIC;

   date_time(widget,font);

  if (! (widget->node->flags & GRUB_WIDGET_FLAG_FIXED_WIDTH))
    widget->width = data->text->common.width;

  if (! (widget->node->flags & GRUB_WIDGET_FLAG_FIXED_HEIGHT))
    widget->height = data->text->common.height;

}

void
date_free (grub_widget_t widget)
{
  struct text_data *data = widget->data;

  grub_menu_region_free ((grub_menu_region_common_t) data->text);
}

void
date_draw (grub_widget_t widget, grub_menu_region_update_list_t *head,
	    int x, int y, int width, int height)
{
  struct text_data *data = widget->data;

  data->text->color = ((widget->node->flags & GRUB_WIDGET_FLAG_SELECTED) ?
		       data->color_selected : data->color);

  if (! (widget->node->flags & GRUB_WIDGET_FLAG_SELECTED))
    grub_menu_region_hide_cursor ();

  grub_menu_region_add_update (head, (grub_menu_region_common_t) data->text,
			       widget->org_x, widget->org_y,
			       x, y, width, height);
}
