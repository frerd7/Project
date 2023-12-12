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

int password_get_data_size (void)
{
  return sizeof (struct password_data);
}

void password_init_size (grub_widget_t widget)
{
  struct password_data *data = widget->data;
  grub_font_t font;
  char *p;

  widget->node->flags |= GRUB_WIDGET_FLAG_TRANSPARENT | GRUB_WIDGET_FLAG_NODE;

  p = grub_widget_get_prop (widget->node, "font");
  font = grub_menu_region_get_font (p);

  p = grub_widget_get_prop (widget->node, "color");
  if (p)
    data->color = grub_menu_parse_color (p, 0, &data->color_selected, 0);

  if (data->color != data->color_selected)
    widget->node->flags |= GRUB_WIDGET_FLAG_DYNAMIC;
  else
    widget->node->flags &= ~GRUB_WIDGET_FLAG_DYNAMIC;

  data->char_width = grub_menu_region_get_text_width (font, "*", 0, 0);
  data->char_height = grub_menu_region_get_text_height (font);

  data->text = grub_menu_region_create_text (font, 0, 0);
  if (! data->text)
    return;

  if (! (widget->node->flags & GRUB_WIDGET_FLAG_FIXED_WIDTH))
    {
      int columns;

      p = grub_widget_get_prop (widget->node, "columns");
      columns = (p) ? grub_strtoul (p, 0, 0) : DEFAULT_COLUMNS;
      widget->width = columns * data->char_width;
    }

  if (! (widget->node->flags & GRUB_WIDGET_FLAG_FIXED_HEIGHT))
    widget->height = data->char_height;
}

void password_free (grub_widget_t widget)
{
  struct password_data *data = widget->data;

  grub_menu_region_free ((grub_menu_region_common_t) data->text);
  grub_free (data->password);
}

void password_draw (grub_widget_t widget, grub_menu_region_update_list_t *head,
	       int x, int y, int width, int height)
{
  struct password_data *data = widget->data;

  data->text->color = ((widget->node->flags & GRUB_WIDGET_FLAG_SELECTED) ?
		       data->color_selected : data->color);

  grub_menu_region_add_update (head, (grub_menu_region_common_t) data->text,
			       widget->org_x, widget->org_y,
			       x, y, width, height);
}

int password_scroll_x (struct password_data *data, int width)
{
  int text_width;

  text_width = data->text->common.width;
  data->x = data->text->common.ofs_x + text_width;

  if ((data->x >= 0) && (data->x + data->char_width <= width))
    return 0;

  width = (width + 1) >> 1;
  if (width > text_width)
    width = text_width;

  data->x = width;
  data->text->common.ofs_x = width - text_width;
  return 1;
}

void password_draw_cursor (grub_widget_t widget)
{
  struct password_data *data = widget->data;

  grub_menu_region_draw_cursor (data->text, data->char_width, data->char_height,
				widget->org_x + data->x, widget->org_y);
}

int password_onkey (grub_widget_t widget, int key)
{
  struct password_data *data = widget->data;

  if ((key == GRUB_TERM_UP) || (key == GRUB_TERM_DOWN) ||
      (key == GRUB_TERM_LEFT) || (key == GRUB_TERM_RIGHT))
    return GRUB_WIDGET_RESULT_DONE;
  else if (key == GRUB_TERM_TAB)
    {
      if (data->modified)
	{
	  char *buf;

	  buf = grub_malloc (data->pos + 1);
	  if (! buf)
	    return grub_errno;

	  grub_memcpy (buf, data->password, data->pos);
	  buf[data->pos] = 0;
	  if (grub_uitree_set_prop (widget->node, "text", buf))
	    return grub_errno;

	  grub_free (buf);
	}
      return GRUB_WIDGET_RESULT_SKIP;
    }
  else if ((key >= 32) && (key < 127))
    {
      if ((data->pos & (STR_INC_STEP - 1)) == 0)
	{
	  data->password = grub_realloc (data->password,
					 data->pos + STR_INC_STEP);
	  if (! data->password)
	    return grub_errno;
	}
      data->password[data->pos] = key;

      data->text = resize_text (data->text, data->pos + 1);
      if (! data->text)
	return grub_errno;

      data->text->text[data->pos++] = '*';
      data->text->text[data->pos] = 0;
      data->text->common.width += data->char_width;
      password_scroll_x (data, widget->width);
      grub_widget_draw (widget->node);
      data->modified = 1;

      return GRUB_WIDGET_RESULT_DONE;
    }
  else if (key == GRUB_TERM_BACKSPACE)
    {
      if (data->pos)
	{
	  data->pos--;
	  data->text->text[data->pos] = 0;
	  data->text->common.width -= data->char_width;
	  password_scroll_x (data, widget->width);
	  grub_widget_draw (widget->node);
	  data->modified = 1;
	}
      return GRUB_WIDGET_RESULT_DONE;
    }
  else
    return GRUB_WIDGET_RESULT_SKIP;
}
