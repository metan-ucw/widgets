//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_RENDER_H__
#define GP_WIDGET_RENDER_H__

#include <poll.h>

#include <core/gp_core.h>
#include <gfx/gp_gfx.h>
#include <text/gp_text.h>
#include <input/gp_timer.h>

#include <gp_widget.h>

typedef struct gp_widget_render_cfg {
	/* colors */
	gp_pixel text_color;
	gp_pixel bg_color;
	gp_pixel fg_color;
	gp_pixel fg2_color;
	gp_pixel sel_color;
	gp_pixel alert_color;

	/* fonts */
	gp_text_style *font;
	gp_text_style *font_bold;

	/* pixel type used for drawing */
	gp_pixel_type pixel_type;

	/* padding between widgets */
	unsigned int padd;

	gp_pixmap *buf;
} gp_widget_render_cfg;

void gp_widget_render_init(void);

typedef struct gp_widget_timer {
	uint32_t (*callback)(void *priv);
	void *priv;
	/* do not touch */
	gp_timer tmr;
} gp_widget_timer;

enum gp_widget_render_timer_flags {
	GP_TIMER_RESCHEDULE = 0x01,
};

/*
 * @callback Function that is called when data are ready on a file descriptor a
 *            non-zero return value removes the poll.
 * @priv A user pointer.
 * @fd File descriptor to be polled for POLLIN.
 * @events Poll events to watch for e.g. POLLIN, POLLPRI, ...
 */
typedef struct gp_widget_poll {
	int (*callback)(struct gp_widget_poll *self);
	void *priv;
	int fd;
	short events;
	short flag;
} gp_widget_poll;

/*
 * Adds a file descriptor to the widgets main loop poll().
 *
 * @poll A structure with a file descriptor and callback.
 */
void gp_widgets_poll_add(struct gp_widget_poll *poll);

/*
 * Removes existing poll.
 *
 * @poll Pointer to a structure to be removed.
 */
void gp_widgets_poll_rem(struct gp_widget_poll *poll);

void gp_widget_render_timer(gp_widget *self, int flags, unsigned int timeout_ms);

void gp_widgets_timer_add(struct gp_widget_timer *tmr, uint32_t expires_ms);


void gp_widgets_main_loop(struct gp_widget *layout, const char *label,
                        void (*init)(void), int argc, char *argv[]);

int gp_widgets_fd(void);

int gp_widgets_process_events(gp_widget *layout);

void gp_widgets_layout_init(gp_widget *layout, const char *win_tittle);

gp_widget *gp_widget_layout_replace(gp_widget *layout);

void gp_widgets_redraw(gp_widget *layout);

void gp_widget_render_zoom(int zoom_inc);

void gp_text_fit(gp_pixmap *pix, const gp_text_style *style,
                gp_coord x, gp_coord y, gp_size w, int align,
		gp_pixel fg_color, gp_pixel bg_color, const char *str);

void gp_fill_rrect_xywh(gp_pixmap *pix, gp_coord x, gp_coord y, gp_size w, gp_size h,
                        gp_pixel bg_color, gp_pixel fg_color, gp_pixel fr_color);

void gp_rrect_xywh(gp_pixmap *pix, gp_coord x, gp_coord y,
                   gp_size w, gp_size h, gp_pixel color);

void gp_triangle_up(gp_pixmap *pix, gp_coord x_center, gp_coord y_center,
                    gp_size base, gp_pixel color);

void gp_triangle_down(gp_pixmap *pix, gp_coord x_center, gp_coord y_center,
                      gp_size base, gp_pixel color);

void gp_triangle_updown(gp_pixmap *pix, gp_coord x_center, gp_coord y_center,
                        gp_size base, gp_pixel color);

#endif /* GP_WIDGET_RENDER_H__ */
