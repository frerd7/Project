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
#include <grub/widget.h>
#include <grub/lib.h>
#include <grub/bitmap_scale.h>
#include <grub/menu_data.h>
#include <grub/parser.h>
#include <grub/lib.h>
#include <grub/charset.h>
#include <grub/command.h>
#include <grub/trig.h>
#include "coreui.h"

struct edit_data
{
  grub_menu_region_text_t *lines;
  int num_lines;
  int max_lines;
  int line;
  int pos;
  int x;
  int y;
  grub_video_color_t color;
  grub_video_color_t color_selected;
  int font_height;
  int font_width;
  int modified;
};

#define REFERENCE_STRING	"m"
#define DEFAULT_MAX_LINES	100
#define DEFAULT_LINES		1
#define LINE_INC_STEP		8

static int
edit_get_data_size (void)
{
  return sizeof (struct edit_data);
}

static void
edit_init_size (grub_widget_t widget)
{
  struct edit_data *data = widget->data;
  grub_font_t font;
  int num, max;
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

  p = grub_widget_get_prop (widget->node, "max_lines");
  data->max_lines = (p) ? grub_strtoul (p, 0, 0) : DEFAULT_MAX_LINES;

  data->font_width = grub_menu_region_get_text_width (font, REFERENCE_STRING,
						      0, 0);
  data->font_height = grub_menu_region_get_text_height (font);

  p = grub_widget_get_prop (widget->node, "text");
  if (! p)
    p = "";

  num = max = 0;
  do
    {
      char *n;

      n = grub_menu_next_field (p, '\n');
      if (num >= max)
	{
	  data->lines = grub_realloc (data->lines, (max + LINE_INC_STEP)
				      * sizeof (data->lines[0]));
	  if (! data->lines)
	    return;
	  grub_memset (data->lines + max, 0,
		       LINE_INC_STEP * sizeof (data->lines[0]));
	  max += LINE_INC_STEP;
	}
      data->lines[num] = grub_menu_region_create_text (font, 0, p);
      if (! data->lines[num])
	return;
      data->lines[num]->common.ofs_y = num * data->font_height;
      grub_menu_restore_field (n, '\n');
      num++;
      p = n;
    } while ((p) && (num != data->max_lines));
  data->num_lines = num;

  if (! (widget->node->flags & GRUB_WIDGET_FLAG_FIXED_WIDTH))
    {
      int columns;

      p = grub_widget_get_prop (widget->node, "columns");
      columns = (p) ? grub_strtoul (p, 0, 0) : DEFAULT_COLUMNS;
      widget->width = columns * data->font_width;
    }

  if (! (widget->node->flags & GRUB_WIDGET_FLAG_FIXED_HEIGHT))
    {
      int lines;

      p = grub_widget_get_prop (widget->node, "lines");
      lines = (p) ? grub_strtoul (p, 0, 0) : DEFAULT_LINES;
      widget->height = lines * data->font_height;
    }
}

static void
edit_free (grub_widget_t widget)
{
  struct edit_data *data = widget->data;
  int i;

  for (i = 0; i < data->num_lines; i++)
    grub_menu_region_free ((grub_menu_region_common_t) data->lines[i]);
  grub_free (data->lines);
}

static void
edit_draw (grub_widget_t widget, grub_menu_region_update_list_t *head,
	    int x, int y, int width, int height)
{
  struct edit_data *data = widget->data;
  grub_video_color_t color;
  grub_menu_region_text_t *p;
  int i;

  color = ((widget->node->flags & GRUB_WIDGET_FLAG_SELECTED) ?
	   data->color_selected : data->color);

  for (i = 0, p = data->lines; i < data->num_lines; i++, p++)
    if (*p)
      {
	(*p)->color = color;
	grub_menu_region_add_update (head,
				     (grub_menu_region_common_t) *p,
				     widget->org_x, widget->org_y,
				     x, y, width, height);
      }
}

static int cursor_off;

static void
edit_draw_cursor (grub_widget_t widget)
{
  struct edit_data *data = widget->data;
  grub_font_t font;
  char *line;
  int cursor_width;

  if (cursor_off)
    return;

  line = data->lines[data->line]->text;
  font = data->lines[data->line]->font;

  cursor_width = (line[data->pos]) ?
    grub_menu_region_get_text_width (font, line + data->pos, 1, 0) :
    data->font_width;

  grub_menu_region_draw_cursor (data->lines[data->line], cursor_width,
				data->font_height, widget->org_x + data->x,
				widget->org_y + data->y);
}

static int
edit_scroll_x (struct edit_data *data, int text_width, int width)
{
  data->x = data->lines[data->line]->common.ofs_x + text_width;

  if ((data->x >= 0) && (data->x + data->font_width <= width))
    return 0;

  width = (width + 1) >> 1;
  if (width > text_width)
    width = text_width;

  data->x = width;
  data->lines[data->line]->common.ofs_x = width - text_width;
  return 1;
}

static void
edit_move_y (struct edit_data *data, int delta)
{
  int i;
  grub_menu_region_text_t *p;

  for (i = 0, p = data->lines; i < data->num_lines; i++, p++)
    if (*p)
      (*p)->common.ofs_y += delta;
}

static int
edit_scroll_y (struct edit_data *data, int height)
{
  int text_height, delta;

  text_height = data->font_height * data->line;
  data->y = data->lines[0]->common.ofs_y + text_height;

  if ((data->lines[0]->common.ofs_y <= 0) &&
      (data->y >= 0) && (data->y + data->font_height <= height))
    return 0;

  height = (height + 1) >> 1;
  if (height > text_height)
    height = text_height;

  delta = height - data->y;
  data->y = height;
  edit_move_y (data, delta);
  return 1;
}

static int
edit_prev_char_width (grub_font_t font, char *line, int pos, int* len)
{
  int width;
  char *p;

  width = 0;
  p = line;
  while (1)
    {
      int w, n;

      w = grub_menu_region_get_text_width (font, p, 1, &n);
      pos -= n;
      if (pos <= 0)
	break;

      p += n;
      width += w;
    }

  *len = p - line;

  return width;
}

static void
draw_region (grub_uitree_t node, int x, int y, int width, int height)
{
  grub_menu_region_update_list_t head;

  head = 0;
  grub_widget_draw_region (&head, node, x, y, width, height);
  grub_menu_region_apply_update (head);
}

static int
edit_handle_key (grub_widget_t widget, int key)
{
  struct edit_data *data = widget->data;
  grub_font_t font;
  char *line;
  int update_x, update_y, update_width, update_height, text_width, scroll_y;

  line = data->lines[data->line]->text;
  font = data->lines[data->line]->font;
  update_x = data->x;
  update_y = data->y;
  update_width = (line[data->pos]) ?
    grub_menu_region_get_text_width (font, line + data->pos, 1, 0) :
    data->font_width;
  update_height = data->font_height;
  text_width = -1;
  scroll_y = 0;

  if (key == GRUB_TERM_LEFT)
    {
      if (! data->pos)
	return GRUB_WIDGET_RESULT_DONE;

      text_width = edit_prev_char_width (font, line, data->pos, &data->pos);
    }
  else if (key == GRUB_TERM_RIGHT)
    {
      int n;

      if (! line[data->pos])
	return GRUB_WIDGET_RESULT_DONE;

      text_width = data->x - data->lines[data->line]->common.ofs_x +
	grub_menu_region_get_text_width (font, line + data->pos, 1, &n);
      data->pos += n;
    }
  else if (key == GRUB_TERM_HOME)
    {
      if (! data->pos)
	return GRUB_WIDGET_RESULT_DONE;

      text_width = data->pos = 0;
    }
  else if (key == GRUB_TERM_END)
    {
      text_width = data->lines[data->line]->common.width;
      data->pos = grub_strlen (line);
    }
  else if ((key == GRUB_TERM_UP) || (key == GRUB_TERM_CTRL_P) ||
	   (key == GRUB_TERM_PPAGE))
    {
      int n;

      if (! data->line)
	return GRUB_WIDGET_RESULT_DONE;

      n = (key == GRUB_TERM_PPAGE) ? widget->height / data->font_height : 1;
      if (n > data->line)
	n = data->line;

      if (! n)
	return GRUB_WIDGET_RESULT_DONE;

      if ((data->line + 1 == data->num_lines) && (! *line))
	{
	  grub_menu_region_free ((grub_menu_region_common_t)
				 data->lines[data->line]);
	  data->lines[data->line] = 0;
	  data->num_lines--;
	}

      data->line -= n;
      scroll_y = 1;
    }
  else if ((key == GRUB_TERM_DOWN) || (key == GRUB_TERM_CTRL_N))
    {
      if (data->max_lines == 1)
	return GRUB_WIDGET_RESULT_DONE;

      data->line++;
      data->y += data->font_height;

      if (data->line >= data->num_lines)
	{
	  data->num_lines++;
	  if ((data->num_lines & (LINE_INC_STEP - 1)) == 0)
	    {
	      data->lines =
		grub_realloc (data->lines,
			      (data->num_lines + LINE_INC_STEP) *
			      sizeof (void *));
	      if (! data->lines)
		return grub_errno;

	      grub_memset (data->lines + data->num_lines, 0,
			   LINE_INC_STEP * sizeof (void *));
	    }
	}

      if (! data->lines[data->line])
	{
	  data->lines[data->line] = grub_menu_region_create_text (font, 0, 0);
	  if (! data->lines[data->line])
	    return grub_errno;

	  data->lines[data->line]->common.ofs_y = data->y;
	}

      scroll_y = 1;
    }
  else if (key == '\r')
    {
      int i;

      if (data->max_lines == 1)
	return GRUB_WIDGET_RESULT_DONE;

      data->line++;
      data->num_lines++;
      if ((data->num_lines & (LINE_INC_STEP - 1)) == 0)
	{
	  data->lines = grub_realloc (data->lines,
				      (data->num_lines + LINE_INC_STEP) *
				      sizeof (void *));
	  if (! data->lines)
	    return grub_errno;

	  grub_memset (data->lines + data->num_lines, 0,
		       LINE_INC_STEP * sizeof (void *));
	}

      for (i = data->num_lines - 1; i > data->line; i--)
	{
	  data->lines[i] = data->lines[i - 1];
	  data->lines[i]->common.ofs_y += data->font_height;
	}

      data->y += data->font_height;
      data->lines[data->line] =
	grub_menu_region_create_text (font, 0, line + data->pos);
      data->lines[data->line]->common.ofs_y = data->y;

      line[data->pos] = 0;
      data->lines[data->line - 1]->common.width =
	grub_menu_region_get_text_width (font, line, 0, 0);
      data->lines[data->line - 1]->common.ofs_x = 0;
      data->x = 0;
      data->pos = 0;

      update_x = 0;
      update_width = widget->width;
      update_height = (data->num_lines - data->line + 1) * data->font_height;

      scroll_y = 1;
      data->modified = 1;
    }
  else if (key == GRUB_TERM_NPAGE)
    {
      int n;

      n = widget->height / data->font_height;
      if (data->line + n >= data->num_lines)
	n = data->num_lines - 1 - data->line;

      if (! n)
	return GRUB_WIDGET_RESULT_DONE;

      data->line += n;
      scroll_y = 1;
    }
  else if ((key >= 32) && (key < 127))
    {
      int len, i;

      len = grub_strlen (line);
      data->lines[data->line] = resize_text (data->lines[data->line], len + 1);
      if (! data->lines[data->line])
	return grub_errno;

      line = data->lines[data->line]->text;

      for (i = len - 1; i >= data->pos; i--)
	line[i + 1] = line[i];
      line[len + 1] = 0;
      line[data->pos] = key;
      data->lines[data->line]->common.width =
	grub_menu_region_get_text_width (font, line, 0, 0);
      text_width = data->x - data->lines[data->line]->common.ofs_x;
      update_width = data->lines[data->line]->common.width - text_width;
      text_width +=
	grub_menu_region_get_text_width (font, line + data->pos, 1, 0);
      data->pos++;
      data->modified = 1;
    }
  else if (key == GRUB_TERM_BACKSPACE)
    {
      int n, delta;
      char *p;

      if (! data->pos)
	{
	  int i, len;

	  if (! data->line)
	    return GRUB_WIDGET_RESULT_DONE;

	  data->line--;
	  data->pos = grub_strlen (data->lines[data->line]->text);
	  len = grub_strlen (line);
	  if (len)
	    {
	      data->lines[data->line] = resize_text (data->lines[data->line],
						     data->pos + len);
	      if (! data->lines[data->line])
		return 1;

	      grub_strcpy (data->lines[data->line]->text + data->pos,
			   line);
	    }
	  grub_menu_region_free ((grub_menu_region_common_t)
				 data->lines[data->line + 1]);
	  for (i = data->line + 1; i < data->num_lines - 1; i++)
	    {
	      data->lines[i] = data->lines[i + 1];
	      data->lines[i]->common.ofs_y -= data->font_height;
	    }
	  data->num_lines--;
	  data->lines[data->num_lines] = 0;

	  data->x = data->lines[data->line]->common.width;
	  data->lines[data->line]->common.width =
	    grub_menu_region_get_text_width (font,
					     data->lines[data->line]->text,
					     0, 0);
	  data->y -= data->font_height;

	  edit_scroll_x (data, data->x, widget->width);
	  update_x = 0;
	  update_width = widget->width;
	  update_y = data->y;
	  update_height = (data->num_lines - data->line + 1) *
	    data->font_height;
	}
      else
	{
	  text_width = edit_prev_char_width (font, line, data->pos, &n);
	  update_width = data->lines[data->line]->common.width -
	    text_width;
	  if (! line[data->pos])
	    update_width += data->font_width;

	  delta = data->pos - n;
	  for (p = line + data->pos; ; p++)
	    {
	      *(p - delta) = *p;
	      if (! *p)
		break;
	    }
	  data->pos = n;
	  data->lines[data->line]->common.width =
	    grub_menu_region_get_text_width (font, line, 0, 0);
	  update_x = data->lines[data->line]->common.ofs_x + text_width;
	}
      data->modified = 1;
    }
  else if ((key == GRUB_TERM_CTRL_X) || (key == GRUB_TERM_TAB))
    {
      if (data->modified)
	{
	  int i, len;
	  char *buf, *p;

	  len = 0;
	  for (i = 0; i < data->num_lines; i++)
	    len += grub_strlen (data->lines[i]->text) + 1;

	  buf = grub_malloc (len);
	  if (! buf)
	    return grub_errno;

	  p = buf;
	  for (i = 0; i < data->num_lines; i++)
	    {
	      grub_strcpy (p, data->lines[i]->text);
	      p += grub_strlen (p);
	      *(p++) = '\n';
	    }
	  *(p - 1) = 0;
	  if (grub_uitree_set_prop (widget->node, "text", buf))
	    return grub_errno;

	  grub_free (buf);
	  data->modified = 0;
	}

      return (key == GRUB_TERM_TAB) ? GRUB_WIDGET_RESULT_SKIP : 0;
    }
  else
    return GRUB_WIDGET_RESULT_SKIP;

  if ((text_width >= 0) && (edit_scroll_x (data, text_width, widget->width)))
    {
      update_x = 0;
      update_width = widget->width;
    }

  if (scroll_y)
    {
      if ((data->max_lines) && (data->line >= data->max_lines))
	{
	  int i, n;

	  n = widget->height / (2 * data->font_height);
	  if (n < 1)
	    n = 1;
	  else if (n > data->line)
	    n = data->line;

	  for (i = 0; i < n; i++)
	    grub_menu_region_free ((grub_menu_region_common_t) data->lines[i]);

	  for (i = 0; i <= data->line - n; i++)
	    data->lines[i] = data->lines[i + n];

	  for (; i <= data->line; i++)
	    data->lines[i] = 0;

	  data->line -= n;
	  data->num_lines -= n;
	}

      data->x = 0;
      data->pos = 0;
      if (edit_scroll_y (data, widget->height))
	{
	  data->x = data->lines[data->line]->common.ofs_x = 0;
	  update_x = 0;
	  update_y = 0;
	  update_width = widget->width;
	  update_height = widget->height;
	}
      else if (edit_scroll_x (data, 0, widget->width))
	{
	  draw_region (widget->node, update_x, update_y,
		       update_width, update_height);
	  update_x = 0;
	  update_y = data->y;
	  update_width = widget->width;
	  update_height = data->font_height;
	}
    }

  draw_region (widget->node, update_x, update_y, update_width, update_height);

  return GRUB_WIDGET_RESULT_DONE;
}

static int
edit_onkey (grub_widget_t widget, int key)
{
  return edit_handle_key (widget, key);
}

static struct grub_widget_class edit_widget_class =
  {
    .name = "edit",
    .get_data_size = edit_get_data_size,
    .init_size = edit_init_size,
    .free = edit_free,
    .draw = edit_draw,
    .draw_cursor = edit_draw_cursor,
    .onkey = edit_onkey,
  };

static grub_widget_t cur_term;

#define GRUB_PROMPT	"grub> "

struct term_data
{
  struct edit_data edit;
  int hist_pos;
};

static int
term_get_data_size (void)
{
  return sizeof (struct term_data);
}

static void
term_free (grub_widget_t widget)
{
  if (cur_term == widget)
    cur_term = 0;

  edit_free (widget);
}

static void
term_draw (grub_widget_t widget, grub_menu_region_update_list_t *head,
	   int x, int y, int width, int height)
{

  if ((! (widget->node->flags & GRUB_WIDGET_FLAG_SELECTED)) &&
      (cur_term == widget))
    cur_term = 0;

  edit_draw (widget, head, x, y, width, height);
}

static void
term_draw_cursor (grub_widget_t widget)
{
  struct edit_data *data = widget->data;

  cur_term = widget;
  if ((data->line + 1 == data->num_lines) &&
      (! data->lines[data->line]->text[0]))
    grub_printf (GRUB_PROMPT);
  else
    {
      int len;

      len = sizeof (GRUB_PROMPT) - 1;
      if ((! grub_strncmp (data->lines[data->line]->text, GRUB_PROMPT, len))
	  && (data->pos < len))
	{
	  data->pos = len;
	  data->x = grub_menu_region_get_text_width (data->lines[0]->font,
						     GRUB_PROMPT, 0, 0);
	}
    }
  edit_draw_cursor (widget);
}

static void
clear_rest (grub_widget_t widget)
{
  struct edit_data *data = widget->data;
  int i;

  for (i = data->line + 1; i < data->num_lines; i++)
    {
      grub_menu_region_free ((grub_menu_region_common_t) data->lines[i]);
      data->lines[i] = 0;
    }

  if (data->num_lines > data->line + 1)
    {
      int height;

      height = (data->num_lines - data->line - 1) * data->font_height;
      draw_region (widget->node, 0, data->y + data->font_height,
		   widget->width, height);
      data->num_lines = data->line + 1;
    }
}

/* A completion hook to print items.  */
static void
print_completion (const char *item, grub_completion_type_t type, int count)
{
  if (count == 0)
    {
      /* If this is the first time, print a label.  */
      const char *what;

      clear_rest (cur_term);
      switch (type)
	{
	case GRUB_COMPLETION_TYPE_COMMAND:
	  what = "commands";
	  break;
	case GRUB_COMPLETION_TYPE_DEVICE:
	  what = "devices";
	  break;
	case GRUB_COMPLETION_TYPE_FILE:
	  what = "files";
	  break;
	case GRUB_COMPLETION_TYPE_PARTITION:
	  what = "partitions";
	  break;
	case GRUB_COMPLETION_TYPE_ARGUMENT:
	  what = "arguments";
	  break;
	default:
	  what = "things";
	  break;
	}

      grub_printf ("\nPossible %s are:\n", what);
    }

  if (type == GRUB_COMPLETION_TYPE_PARTITION)
    {
      grub_print_device_info (item);
      grub_errno = GRUB_ERR_NONE;
    }
  else
    {
      struct edit_data *data = cur_term->data;
      int width;

      width = data->x + data->font_width +
	grub_menu_region_get_text_width (data->lines[0]->font, item, 0, 0);
      if (width >= cur_term->width)
	edit_handle_key (cur_term, GRUB_TERM_DOWN);
      grub_printf (" %s", item);
    }
}

static int
term_onkey (grub_widget_t widget, int key)
{
  struct term_data *data = widget->data;
  int has_prompt;

  has_prompt = (! grub_strncmp (data->edit.lines[data->edit.line]->text,
				GRUB_PROMPT, sizeof (GRUB_PROMPT) - 1));

  if ((key == '\r') && (has_prompt))
    {
      char *cmd;

      clear_rest (widget);
      cmd = data->edit.lines[data->edit.line]->text +
	sizeof (GRUB_PROMPT) - 1;
      edit_handle_key (widget, GRUB_TERM_DOWN);
      if (*cmd)
	{
	  int len;
	  grub_uint32_t *ustr, *last;

	  len = grub_utf8_to_ucs4_alloc (cmd, &ustr, &last);
	  if (len != -1)
	    {
	      if (! data->hist_pos)
		grub_history_add (ustr, len);
	      else
		grub_history_replace (data->hist_pos - 1, ustr, len);
	      data->hist_pos = 0;
	      grub_free (ustr);
	    }

	  grub_parser_execute (cmd);
	  if (grub_widget_refresh)
	    return GRUB_WIDGET_RESULT_DONE;

	  if (data->edit.pos)
	    edit_handle_key (widget, GRUB_TERM_DOWN);
	  if (grub_errno)
	    {
	      grub_print_error ();
	      grub_errno = 0;
	    }
	}
      return GRUB_WIDGET_RESULT_DONE;
    }
  else if (((key == GRUB_TERM_UP) || (key == GRUB_TERM_DOWN)) && (has_prompt))
    {
      int text_width;
      char *line;

      if (key == GRUB_TERM_UP)
	{
	  if (data->hist_pos < grub_history_used ())
	    data->hist_pos++;
	  else
	    return GRUB_WIDGET_RESULT_DONE;
	}
      else
	{
	  if (data->hist_pos > 0)
	    data->hist_pos--;
	  else
	    return GRUB_WIDGET_RESULT_DONE;
	}

      if (data->hist_pos > 0)
	{
	  grub_uint32_t *ustr, *p;
	  int len;

	  ustr = grub_history_get (data->hist_pos - 1);
	  len = 0;
	  p = ustr;
	  while (*p)
	    {
	      p++;
	      len++;
	    }
	  line = grub_ucs4_to_utf8_alloc (ustr, len);
	}
      else
	line = grub_strdup ("");

      data->edit.lines[data->edit.line] =
	resize_text (data->edit.lines[data->edit.line],
		     grub_strlen (line) + sizeof (GRUB_PROMPT) - 1);

      if (! data->edit.lines[data->edit.line])
	{
	  grub_free (line);
	  return grub_errno;
	}

      grub_strcpy (data->edit.lines[data->edit.line]->text
		   + sizeof (GRUB_PROMPT) - 1, line);
      grub_free (line);
      line = data->edit.lines[data->edit.line]->text;
      data->edit.pos = grub_strlen (line);
      text_width = data->edit.lines[data->edit.line]->common.width =
	grub_menu_region_get_text_width (data->edit.lines[0]->font, line,
					 0, 0);
      data->edit.lines[data->edit.line]->common.ofs_x = 0;
      edit_scroll_x (&data->edit, text_width, widget->width);
      draw_region (widget->node, 0, data->edit.y, widget->width,
		   data->edit.font_height);

      return GRUB_WIDGET_RESULT_DONE;
    }
  else if (key == GRUB_TERM_TAB)
    {
      char *insert, backup;
      int restore, pos, size, len, ilen, i;
      int update_x, update_y, update_width, update_height, text_width;
      char *line;

      update_x = data->edit.x;
      update_y = data->edit.y;

      pos = data->edit.pos;
      line = data->edit.lines[data->edit.line]->text;
      backup = line[pos];
      line[pos] = 0;

      insert = grub_complete (line + sizeof (GRUB_PROMPT) - 1,
			      &restore, print_completion);

      for (i = data->edit.line; i >= 0; i--)
	if (data->edit.lines[i]->text == line)
	  break;

      if (i < 0)
	return GRUB_WIDGET_RESULT_DONE;

      data->edit.line = i;
      data->edit.pos = pos;
      line[pos] = backup;

      if (insert)
	{

	  len = grub_strlen (line);
	  ilen = grub_strlen (insert);
	  size = (sizeof (struct grub_menu_region_text) + len + ilen
		  + 1 + 15) & ~15;
	  data->edit.lines[i] = grub_realloc (data->edit.lines[i], size);
	  if (! data->edit.lines[i])
	    return grub_errno;

	  line = data->edit.lines[i]->text;

	  for (i = len - 1; i >= pos; i--)
	    line[i + ilen] = line[i];

	  line[len + ilen] = 0;

	  for (i = 0; i < ilen; i++)
	    line[pos + i] = insert[i];

	  grub_free (insert);

	  data->edit.lines[data->edit.line]->common.width =
	    grub_menu_region_get_text_width (data->edit.lines[0]->font,
					     line, 0, 0);
	}
      else
	ilen = 0;

      text_width = update_x - data->edit.lines[data->edit.line]->common.ofs_x;
      update_width = data->edit.lines[data->edit.line]->common.width
	- text_width;
      if (ilen)
	{
	  text_width +=
	    grub_menu_region_get_text_width (data->edit.lines[0]->font,
					     line + pos, ilen, 0);
	  data->edit.pos += ilen;
	}

      if (edit_scroll_x (&data->edit, text_width, widget->width))
	{
	  update_x = 0;
	  update_width = widget->width;
	}

      if (edit_scroll_y (&data->edit, widget->height))
	{
	  update_x = 0;
	  update_y = 0;
	  update_width = widget->width;
	  update_height = widget->height;
	}
      else
	update_height = data->edit.font_height;

      draw_region (widget->node, update_x, update_y,
		   update_width, update_height);
      return GRUB_WIDGET_RESULT_DONE;
    }
  else if (key == GRUB_TERM_CTRL_X)
    return GRUB_WIDGET_RESULT_SKIP;
  else
    {
      if (((has_prompt) &&
	   (data->edit.pos <= (int) sizeof (GRUB_PROMPT) - 1)) &&
	  (key == GRUB_TERM_BACKSPACE))
	return GRUB_WIDGET_RESULT_DONE;

      return edit_onkey (widget, key);
    }
}

static struct grub_widget_class term_widget_class =
  {
    .name = "term",
    .get_data_size = term_get_data_size,
    .init_size = edit_init_size,
    .free = term_free,
    .draw = term_draw,
    .draw_cursor = term_draw_cursor,
    .onkey = term_onkey,
  };

static grub_ssize_t
grub_gfxmenu_getcharwidth (grub_uint32_t c __attribute__((unused)))
{
  return 1;
}

static grub_uint16_t
grub_gfxmenu_getwh (void)
{
  int width;

  if (cur_term)
    {
      struct edit_data *data;

      data = cur_term->data;
      width = cur_term->inner_width / data->font_width;
    }
  else
    width = 0xff;

  return (width << 8) + 0xff;
}

static grub_uint16_t
grub_gfxmenu_getxy (void)
{
  struct edit_data *data;

  if (! cur_term)
    return 0;

  data = cur_term->data;
  return (data->pos << 8) | (data->line);
}

static void
grub_gfxmenu_gotoxy (grub_uint8_t x __attribute__((unused)), grub_uint8_t y)
{
  struct edit_data *data;

  if (! cur_term)
    return;

  data = cur_term->data;
  if (y == data->line + 1)
    edit_handle_key (cur_term, GRUB_TERM_DOWN);
}

static void
grub_gfxmenu_cls (void)
{
  struct edit_data *data;
  grub_font_t font;
  int i;

  if (! cur_term)
    return;

  data = cur_term->data;

  data->x = 0;
  data->y = 0;
  data->pos = 0;
  data->line = 0;
  data->pos = 0;
  font = data->lines[0]->font;
  for (i = 0; i < data->num_lines; i++)
    {
      grub_menu_region_free ((grub_menu_region_common_t)
			     data->lines[i]);
      data->lines[i] = 0;
    }
  data->lines[0] = grub_menu_region_create_text (font, 0, 0);
  data->num_lines = 1;
  grub_widget_draw (cur_term->node);
}

static void
grub_gfxmenu_setcolorstate (grub_term_color_state state)
{
  (void) state;
}

static void
grub_gfxmenu_setcolor (grub_uint8_t normal_color, grub_uint8_t highlight_color)
{
  (void) normal_color;
  (void) highlight_color;
}

static void
grub_gfxmenu_getcolor (grub_uint8_t *normal_color,
		       grub_uint8_t *highlight_color)
{
  *normal_color = 0;
  *highlight_color = 0;
}

static void
grub_gfxmenu_setcursor (int on)
{
  cursor_off = ! on;
}

static void
grub_gfxmenu_refresh (void)
{
}

static void
grub_gfxmenu_putchar (grub_uint32_t c)
{
  if (! cur_term)
    return;

  if (c == '\n')
    edit_handle_key (cur_term, GRUB_TERM_DOWN);
  else if (c != '\r')
    {
      int width;
      struct edit_data *data;

      data = cur_term->data;
      width = data->x + data->font_width +
	grub_menu_region_get_text_width (data->lines[0]->font,
					 (char *) &c, 1, 0);
      if (width > cur_term->width)
	edit_handle_key (cur_term, GRUB_TERM_DOWN);

      edit_handle_key (cur_term, c);
    }
}

struct grub_term_output grub_gfxmenu_term =
  {
    .name = "gfxmenu",
    .putchar = grub_gfxmenu_putchar,
    .getcharwidth = grub_gfxmenu_getcharwidth,
    .getwh = grub_gfxmenu_getwh,
    .getxy = grub_gfxmenu_getxy,
    .gotoxy = grub_gfxmenu_gotoxy,
    .cls = grub_gfxmenu_cls,
    .setcolorstate = grub_gfxmenu_setcolorstate,
    .setcolor = grub_gfxmenu_setcolor,
    .getcolor = grub_gfxmenu_getcolor,
    .setcursor = grub_gfxmenu_setcursor,
    .refresh = grub_gfxmenu_refresh,
    .flags = 0,
    .next = 0
  };

GRUB_MOD_INIT(coreui)
{
  grub_widget_class_register (&screen_widget_class);
  grub_widget_class_register (&panel_widget_class);
  grub_widget_class_register (&image_widget_class);
  grub_widget_class_register (&text_widget_class);
  grub_widget_class_register (&password_widget_class);
  grub_widget_class_register (&progressbar_widget_class);
  grub_widget_class_register (&circular_progress_widget_class);
  grub_widget_class_register (&edit_widget_class);
  grub_widget_class_register (&term_widget_class);
  grub_widget_class_register (&date_widget_class);
  grub_term_register_output ("gfxmenu", &grub_gfxmenu_term);
}

GRUB_MOD_FINI(coreui)
{
  grub_widget_class_unregister (&screen_widget_class);
  grub_widget_class_unregister (&panel_widget_class);
  grub_widget_class_unregister (&image_widget_class);
  grub_widget_class_unregister (&text_widget_class);
  grub_widget_class_unregister (&password_widget_class);
  grub_widget_class_unregister (&progressbar_widget_class);
  grub_widget_class_unregister (&circular_progress_widget_class);
  grub_widget_class_unregister (&edit_widget_class);
  grub_widget_class_unregister (&term_widget_class);
  grub_widget_class_unregister (&date_widget_class);
  grub_term_unregister_output (&grub_gfxmenu_term);
}
