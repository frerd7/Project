#ifndef COREUI_CLASS
#define COREUI_CLASS	1

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

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-function"

#define INDEX_TOP_LEFT		0
#define INDEX_TOP		1
#define INDEX_TOP_RIGHT		2
#define INDEX_LEFT		3
#define INDEX_RIGHT		4
#define INDEX_BOTTOM_LEFT	5
#define INDEX_BOTTOM		6
#define INDEX_BOTTOM_RIGHT	7
#define INDEX_BACKGROUND	8
#define INDEX_SELECTED		9
#define INDEX_BORDER_TOP	18
#define INDEX_BORDER_LEFT	19
#define INDEX_BORDER_RIGHT	20
#define INDEX_BORDER_BOTTOM	21
#define INDEX_COUNT		22

#define MARGIN_FINI	0
#define MARGIN_WIDTH	1
#define MARGIN_HEIGHT	2

#define STR_INC_STEP		8
#define DEFAULT_COLUMNS		20


static char *border_names[8] =
  {
    "top_left",
    "top",
    "top_right",
    "left",
    "right",
    "bottom_left",
    "bottom",
    "bottom_right"
  };

static grub_menu_region_common_t
get_bitmap (grub_widget_t widget, const char *name,
	    int fallback, grub_menu_region_common_t *bitmap_selected)
{
  char *p;
  p = grub_widget_get_prop (widget->node, name);
  return grub_menu_parse_bitmap (p, (fallback) ? 0 : -1, bitmap_selected);
}

void adjust_margin (grub_widget_t widget, const char *name, int mode);

static grub_menu_region_text_t
resize_text (grub_menu_region_text_t text, int len)
{
  int size;
  size = (sizeof (struct grub_menu_region_text) + len + 1 + 15) & ~15;
  return grub_realloc (text, size);
}

struct password_data
{
  grub_menu_region_text_t text;
  grub_video_color_t color;
  grub_video_color_t color_selected;
  char *password;
  int x;
  int pos;
  int char_width;
  int char_height;
  int modified;
};

struct panel_data
{
  grub_menu_region_common_t bitmaps[INDEX_COUNT];
  grub_video_color_t color;
  grub_video_color_t color_selected;
  grub_uint32_t fill;
  grub_uint32_t fill_selected;
};

/* image object */
int image_get_data_size (void);
void image_init_size (grub_widget_t widget);
void image_fini_size (grub_widget_t widget);
void image_free (grub_widget_t widget);
void image_draw (grub_widget_t widget, grub_menu_region_update_list_t *head,
	    int x, int y, int width, int height);

static struct grub_widget_class image_widget_class =
  {
    .name = "image",
    .get_data_size = image_get_data_size,
    .init_size = image_init_size,
    .fini_size = image_fini_size,
    .free = image_free,
    .draw = image_draw
  };

/* text object */
int text_get_data_size (void);
void text_init_size (grub_widget_t widget);
void text_free (grub_widget_t widget);
void text_draw (grub_widget_t widget, grub_menu_region_update_list_t *head,
	    int x, int y, int width, int height);

static struct grub_widget_class text_widget_class =
  {
    .name = "text",
    .get_data_size = text_get_data_size,
    .init_size = text_init_size,
    .free = text_free,
    .draw = text_draw
  };

/* progressbar object */
int
progressbar_get_data_size (void);
void
progressbar_init_size (grub_widget_t widget);
void
progressbar_fini_size (grub_widget_t widget);
void
progressbar_free (grub_widget_t widget);
void
progressbar_draw (grub_widget_t widget, grub_menu_region_update_list_t *head,
	    int x, int y, int width, int height);
void
progressbar_set_timeout (grub_widget_t widget, int total, int left);

static struct grub_widget_class progressbar_widget_class =
  {
    .name = "progressbar",
    .get_data_size = progressbar_get_data_size,
    .init_size = progressbar_init_size,
    .fini_size = progressbar_fini_size,
    .free = progressbar_free,
    .draw = progressbar_draw,
    .set_timeout = progressbar_set_timeout
  };

/* circular_progress */
void circular_progress_set_timeout (grub_widget_t widget, int total, int left);
void circular_progress_draw (grub_widget_t widget,
			grub_menu_region_update_list_t *head,
			int x, int y, int width, int height);
void circular_progress_free (grub_widget_t widget);
void circular_progress_fini_size (grub_widget_t widget);
void circular_progress_init_size (grub_widget_t widget);
int circular_progress_get_data_size (void);

static struct grub_widget_class circular_progress_widget_class =
  {
    .name = "circular_progress",
    .get_data_size = circular_progress_get_data_size,
    .init_size = circular_progress_init_size,
    .fini_size = circular_progress_fini_size,
    .free = circular_progress_free,
    .draw = circular_progress_draw,
    .set_timeout = circular_progress_set_timeout
  };

/* password style object */
int password_onkey (grub_widget_t widget, int key);
void password_draw_cursor (grub_widget_t widget);
int password_scroll_x (struct password_data *data, int width);
void password_draw (grub_widget_t widget, grub_menu_region_update_list_t *head,
	       int x, int y, int width, int height);
void password_free (grub_widget_t widget);
void password_init_size (grub_widget_t widget);
int password_get_data_size (void);

static struct grub_widget_class password_widget_class =
  {
    .name = "password",
    .get_data_size = password_get_data_size,
    .init_size = password_init_size,
    .free = password_free,
    .draw = password_draw,
    .draw_cursor = password_draw_cursor,
    .onkey = password_onkey,
  };

/* panel screen */
void panel_free (grub_widget_t widget);
void panel_draw (grub_widget_t widget, grub_menu_region_update_list_t *head,
	    int x, int y, int width, int height);
void panel_fini_size (grub_widget_t widget);
void panel_init_size (grub_widget_t widget);
void get_border_size (struct panel_data *data, int *dx1, int *dy1,
		 int *dx2, int *dy2);
void
resize_border (grub_widget_t widget, grub_menu_region_common_t *bitmaps,
	       int index, int x, int y, int width, int height);
int panel_get_data_size (void);

static struct grub_widget_class panel_widget_class =
  {
    .name = "panel",
    .get_data_size = panel_get_data_size,
    .init_size = panel_init_size,
    .fini_size = panel_fini_size,
    .free = panel_free,
    .draw = panel_draw
  };

/* screen */
int screen_get_data_size (void);
void screen_init_size (grub_widget_t widget);
void screen_fini_size (grub_widget_t widget);
void screen_free (grub_widget_t widget);
void screen_draw (grub_widget_t widget, grub_menu_region_update_list_t *head,
	     int x, int y, int width, int height);

static struct grub_widget_class screen_widget_class =
  {
    .name = "screen",
    .get_data_size = screen_get_data_size,
    .init_size = screen_init_size,
    .fini_size = screen_fini_size,
    .free = screen_free,
    .draw = screen_draw
  };

/* date time */

void
date_draw (grub_widget_t widget, grub_menu_region_update_list_t *head,
	    int x, int y, int width, int height);
void
date_free (grub_widget_t widget);

void
date_init_size (grub_widget_t widget);

int
date_get_data_size (void);

static struct grub_widget_class date_widget_class =
  {
    .name = "date",
    .get_data_size = date_get_data_size,
    .init_size = date_init_size,
    .free = date_free,
    .draw = date_draw,
  };

#endif // COREUI_CLASS




