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

int panel_get_data_size (void)
{
  return sizeof (struct panel_data);
}

void get_border_size (struct panel_data *data, int *dx1, int *dy1,
		 int *dx2, int *dy2)
{
  int i;

  *dx1 = *dy1 = *dx2 = *dy2 = 0;
  for (i = INDEX_SELECTED; i >= 0; i -= INDEX_SELECTED)
    {
      if (data->bitmaps[INDEX_LEFT + i])
	*dx1 = data->bitmaps[INDEX_LEFT + i]->width;

      if (data->bitmaps[INDEX_TOP + i])
	*dy1 = data->bitmaps[INDEX_TOP + i]->height;

      if (data->bitmaps[INDEX_RIGHT + i])
	*dx2 = (data->bitmaps[INDEX_RIGHT + i])->width;

      if (data->bitmaps[INDEX_BOTTOM + i])
	*dy2 = data->bitmaps[INDEX_BOTTOM + i]->height;
    }
}

void
panel_init_size (grub_widget_t widget)
{
  struct panel_data *data = widget->data;
  int border_size, size;
  char *p;
  int i;
  int dx1, dy1, dx2, dy2;

  for (i = INDEX_TOP_LEFT; i <= INDEX_BOTTOM_RIGHT; i++)
    data->bitmaps[i] = get_bitmap (widget, border_names[i - INDEX_TOP_LEFT],
				   0, &data->bitmaps[i + INDEX_SELECTED]);

  data->bitmaps[INDEX_BACKGROUND] =
    get_bitmap (widget, "background", 0,
		&data->bitmaps[INDEX_BACKGROUND + INDEX_SELECTED]);

  p = grub_widget_get_prop (widget->node, "border_color");
  data->fill = data->fill_selected = 0;
  if (p)
    data->color = grub_menu_parse_color (p, &data->fill,
					 &data->color_selected,
					 &data->fill_selected);

  widget->node->flags |= GRUB_WIDGET_FLAG_DYNAMIC;
  if ((data->fill == data->fill_selected) &&
      (data->color == data->color_selected))
    {
      for (i = INDEX_TOP_LEFT + INDEX_SELECTED;
	   i <= INDEX_BACKGROUND + INDEX_SELECTED; i++)
	if (data->bitmaps[i])
	  break;

      if (i > INDEX_BACKGROUND + INDEX_SELECTED)
	widget->node->flags &= ~GRUB_WIDGET_FLAG_DYNAMIC;
    }

  p = grub_widget_get_prop (widget->node, "border_size");
  border_size = (p) ? grub_menu_parse_size (p, 0, 1) : 0;

  p = grub_widget_get_prop (widget->node, "border_left");
  size = (p) ? grub_menu_parse_size (p, 0, 1) : border_size;
  if (size > 0)
    {
      data->bitmaps[INDEX_BORDER_LEFT] = (grub_menu_region_common_t)
	grub_menu_region_create_rect (size, 0, 0, 0);
    }

  p = grub_widget_get_prop (widget->node, "border_right");
  size = (p) ? grub_menu_parse_size (p, 0, 1) : border_size;
  if (size > 0)
    {
      data->bitmaps[INDEX_BORDER_RIGHT] = (grub_menu_region_common_t)
	grub_menu_region_create_rect (size, 0, 0, 0);
    }

  p = grub_widget_get_prop (widget->node, "border_top");
  size = (p) ? grub_menu_parse_size (p, 0, 0) : border_size;
  if (size > 0)
    {
      data->bitmaps[INDEX_BORDER_TOP] = (grub_menu_region_common_t)
	grub_menu_region_create_rect (0, size, 0, 0);
    }

  p = grub_widget_get_prop (widget->node, "border_bottom");
  size = (p) ? grub_menu_parse_size (p, 0, 0) : border_size;
  if (size > 0)
    {
      data->bitmaps[INDEX_BORDER_BOTTOM] = (grub_menu_region_common_t)
	grub_menu_region_create_rect (0, size, 0, 0);
    }

  widget->node->flags |= GRUB_WIDGET_FLAG_TRANSPARENT;

  get_border_size (data, &dx1, &dy1, &dx2, &dy2);

  if (! (widget->node->flags & GRUB_WIDGET_FLAG_FIXED_WIDTH))
    {
      widget->width += dx1 + dx2;

      if (data->bitmaps[INDEX_BORDER_LEFT])
	widget->width += data->bitmaps[INDEX_BORDER_LEFT]->width;

      if (data->bitmaps[INDEX_BORDER_RIGHT])
	widget->width += data->bitmaps[INDEX_BORDER_RIGHT]->width;

      adjust_margin (widget, "padding", MARGIN_WIDTH);
      adjust_margin (widget, "margin", MARGIN_WIDTH);
    }

  if (! (widget->node->flags & GRUB_WIDGET_FLAG_FIXED_HEIGHT))
    {
      widget->height += dy1 + dy2;

      if (data->bitmaps[INDEX_BORDER_TOP])
	widget->height += data->bitmaps[INDEX_BORDER_TOP]->height;

      if (data->bitmaps[INDEX_BORDER_BOTTOM])
	widget->height += data->bitmaps[INDEX_BORDER_BOTTOM]->height;

      adjust_margin (widget, "padding", MARGIN_HEIGHT);
      adjust_margin (widget, "margin", MARGIN_HEIGHT);
    }
}

void
resize_border (grub_widget_t widget, grub_menu_region_common_t *bitmaps,
	       int index, int x, int y, int width, int height)
{
  if (bitmaps[index])
    {
      grub_menu_region_scale (bitmaps[index], width, height);
      bitmaps[index]->ofs_x = x + widget->inner_x;
      bitmaps[index]->ofs_y = y + widget->inner_y;
    }

  if (bitmaps[index + INDEX_SELECTED])
    {
      grub_menu_region_scale (bitmaps[index + INDEX_SELECTED],
			      width, height);
      bitmaps[index + INDEX_SELECTED]->ofs_x = x + widget->inner_x;
      bitmaps[index + INDEX_SELECTED]->ofs_y = y + widget->inner_y;
    }
}

void
panel_fini_size (grub_widget_t widget)
{
  struct panel_data *data = widget->data;
  int dx1, dy1, dx2, dy2, bw1, bh1, bw2, bh2, width, height;

  adjust_margin (widget, "padding", MARGIN_FINI);

  get_border_size (data, &dx1, &dy1, &dx2, &dy2);

  bw1 = (data->bitmaps[INDEX_BORDER_LEFT]) ?
    data->bitmaps[INDEX_BORDER_LEFT]->width : 0;
  bw2 = (data->bitmaps[INDEX_BORDER_RIGHT]) ?
    data->bitmaps[INDEX_BORDER_RIGHT]->width : 0;
  bh1 = (data->bitmaps[INDEX_BORDER_TOP]) ?
    data->bitmaps[INDEX_BORDER_TOP]->height : 0;
  bh2 = (data->bitmaps[INDEX_BORDER_BOTTOM]) ?
    data->bitmaps[INDEX_BORDER_BOTTOM]->height : 0;

  width = widget->inner_width - dx1 - dx2 - bw1 - bw2;
  height = widget->inner_height - dy1 - dy2 - bh1 - bh2;

  resize_border (widget, data->bitmaps, INDEX_TOP_LEFT,
		 bw1, bh1, dx1, dy1);
  resize_border (widget, data->bitmaps, INDEX_TOP,
		 bw1 + dx1, bh1, width, dy1);
  resize_border (widget, data->bitmaps, INDEX_TOP_RIGHT,
		 bw1 + dx1 + width, bh1, dx2, dy1);
  resize_border (widget, data->bitmaps, INDEX_LEFT,
		 bw1, bh1 + dy1, dx1, height);
  resize_border (widget, data->bitmaps, INDEX_RIGHT,
		 bw1 + dx1 + width, bh1 + dy1, dx2, height);
  resize_border (widget, data->bitmaps, INDEX_BOTTOM_LEFT,
		 bw1, bh1 + dy1 + height, dx1, dy2);
  resize_border (widget, data->bitmaps, INDEX_BOTTOM,
		 bw1 + dx1, bh1 + dy1 + height, width, dy2);
  resize_border (widget, data->bitmaps, INDEX_BOTTOM_RIGHT,
		 bw1 + dx1 + width, bh1 + dy1 + height, dx2, dy2);
  resize_border (widget, data->bitmaps, INDEX_BACKGROUND,
		 bw1 + dx1, bh1 + dy1, width, height);

  if (data->bitmaps[INDEX_BORDER_TOP])
    {
      data->bitmaps[INDEX_BORDER_TOP]->ofs_x = widget->inner_x;
      data->bitmaps[INDEX_BORDER_TOP]->ofs_y = widget->inner_y;
      data->bitmaps[INDEX_BORDER_TOP]->width = widget->inner_width;
    }

  if (data->bitmaps[INDEX_BORDER_BOTTOM])
    {
      data->bitmaps[INDEX_BORDER_BOTTOM]->ofs_x = widget->inner_x;
      data->bitmaps[INDEX_BORDER_BOTTOM]->ofs_y = widget->inner_y +
	widget->inner_height - bh2;
      data->bitmaps[INDEX_BORDER_BOTTOM]->width = widget->inner_width;
    }

  if (data->bitmaps[INDEX_BORDER_LEFT])
    {
      data->bitmaps[INDEX_BORDER_LEFT]->ofs_x = widget->inner_x;
      data->bitmaps[INDEX_BORDER_LEFT]->ofs_y = widget->inner_y + bh1;
      data->bitmaps[INDEX_BORDER_LEFT]->height =
	widget->inner_height - bh1 - bh2;
    }

  if (data->bitmaps[INDEX_BORDER_RIGHT])
    {
      data->bitmaps[INDEX_BORDER_RIGHT]->ofs_x = widget->inner_x +
	widget->inner_width - bw2;
      data->bitmaps[INDEX_BORDER_RIGHT]->ofs_y = widget->inner_y + bh1;
      data->bitmaps[INDEX_BORDER_RIGHT]->height =
	widget->inner_height - bh1 - bh2;
    }

  widget->inner_width = width;
  widget->inner_height = height;
  widget->inner_x += bw1 + dx1;
  widget->inner_y += bh1 + dy1;

  adjust_margin (widget, "margin", MARGIN_FINI);
}

void
panel_free (grub_widget_t widget)
{
  struct panel_data *data = widget->data;
  int i;

  for (i = 0; i < INDEX_COUNT; i++)
    grub_menu_region_free (data->bitmaps[i]);
}

void
panel_draw (grub_widget_t widget, grub_menu_region_update_list_t *head,
	    int x, int y, int width, int height)
{
  struct panel_data *data = widget->data;
  int i, ofs;

  for (i = INDEX_BORDER_TOP; i <= INDEX_BORDER_BOTTOM; i++)
    if (data->bitmaps[i])
      {
	if (widget->node->flags & GRUB_WIDGET_FLAG_SELECTED)
	  {
	    ((grub_menu_region_rect_t) data->bitmaps[i])->color =
	      data->color_selected;
	    ((grub_menu_region_rect_t) data->bitmaps[i])->fill =
	      data->fill_selected;
	  }
	else
	  {
	    ((grub_menu_region_rect_t) data->bitmaps[i])->color =
	      data->color;
	    ((grub_menu_region_rect_t) data->bitmaps[i])->fill =
	      data->fill;
	  }

	grub_menu_region_add_update (head, data->bitmaps[i],
				     widget->org_x, widget->org_y,
				     x, y, width, height);
      }

  ofs = (widget->node->flags & GRUB_WIDGET_FLAG_SELECTED) ? INDEX_SELECTED : 0;
  for (i = INDEX_TOP_LEFT; i <= INDEX_BACKGROUND; i++)
    if (data->bitmaps[i + ofs])
      grub_menu_region_add_update (head, data->bitmaps[i + ofs],
				   widget->org_x, widget->org_y,
				   x, y, width, height);
    else
      grub_menu_region_add_update (head, data->bitmaps[i],
				   widget->org_x, widget->org_y,
				   x, y, width, height);
}
