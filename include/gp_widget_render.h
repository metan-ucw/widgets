//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_RENDER_H__
#define GP_WIDGET_RENDER_H__

#include <core/GP_Core.h>
#include <gfx/GP_Gfx.h>
#include <text/GP_Text.h>
#include <input/GP_Timer.h>

#include <gp_widget.h>

struct gp_widget_render_info {
	gp_pixel text_color;
	gp_pixel bg_color;
	gp_pixel fg_color;
	gp_pixel sel_color;
	gp_pixel alert_color;
	gp_text_style *font;
	gp_text_style *font_bold;
	unsigned int padd;
};

extern const struct gp_widget_render_info *cfg;

struct gp_widget_render {
	gp_pixmap *buf;
};

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

void gp_widget_render_timer(gp_widget *self, int flags, unsigned int timeout_ms);

void gp_widgets_timer_add(struct gp_widget_timer *tmr, uint32_t expires_ms);

void gp_widgets_main_loop(struct gp_widget *layout, const char *label,
                        void (*init)(void));

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
