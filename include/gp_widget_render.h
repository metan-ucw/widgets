//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2020 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_RENDER_H__
#define GP_WIDGET_RENDER_H__

#include <core/gp_core.h>
#include <gfx/gp_gfx.h>
#include <text/gp_text.h>
#include <input/gp_timer.h>

#include <gp_widget.h>
#include <gp_bbox.h>

typedef struct gp_widget_render_ctx {
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

	gp_text_style *font_big;
	gp_text_style *font_big_bold;

	/* pixel type used for drawing */
	gp_pixel_type pixel_type;

	/* padding between widgets */
	unsigned int padd;

	/* area to update on a screen after a call to gp_widget_render() */
	gp_bbox *flip;

	/* passed down if only part of the layout has to be rendered */
	gp_bbox *bbox;

	gp_pixmap *buf;
} gp_widget_render_ctx;

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

extern struct gp_fds *gp_widgets_fds;

void gp_widget_render_timer(gp_widget *self, int flags, unsigned int timeout_ms);
void gp_widget_render_timer_cancel(gp_widget *self);

const gp_widget_render_ctx *gp_widgets_render_ctx(void);

/*
 * Parses options, returns positional arguments, e.g. paths.
 */
void gp_widgets_getopt(int *argc, char **argv[]);

/*
 * Register application event callback.
 *
 * All input events that are not handled by the widget library are passed to
 * the callback registered by this function.
 */
void gp_widgets_register_callback(int (*event_callback)(gp_event *));

void gp_widgets_main_loop(struct gp_widget *layout, const char *label,
                        void (*init)(void), int argc, char *argv[]);

int gp_widgets_fd(void);

int gp_widgets_process_events(gp_widget *layout);

void gp_widgets_layout_init(gp_widget *layout, const char *win_tittle);

gp_widget *gp_widget_layout_replace(gp_widget *layout);

void gp_widgets_redraw(gp_widget *layout);

void gp_widget_render_zoom(int zoom_inc);

#endif /* GP_WIDGET_RENDER_H__ */
