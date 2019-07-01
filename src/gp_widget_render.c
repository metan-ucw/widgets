//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <string.h>
#include <core/GP_Debug.h>
#include <core/GP_Common.h>
#include <backends/GP_Backends.h>
#include <gp_widget_render.h>
#include <gp_widget_ops.h>

static struct gp_text_style font = {
	.pixel_xmul = 1,
	.pixel_ymul = 1,
	.font = &gp_default_font,
};

static struct gp_text_style font_bold = {
	.pixel_xmul = 2,
	.pixel_ymul = 2,
	.pixel_xspace = -1,
	.pixel_yspace = -1,
	.font = &gp_default_font,
};

struct gp_widget_render_info gp_widget_render_info_ = {
	.text_color = 0,
	.bg_color = 0xdddddd,
	.fg_color = 0xeeeeee,
	.sel_color = 0x5555ff,
	.alert_color = 0xff5555,
	.font = &font,
	.font_bold = &font_bold,
	.padd = 4,
};

#define FONT "/usr/share/fonts/droid/DroidSans.ttf"
#define FONT_BOLD "/usr/share/fonts/droid/DroidSans-Bold.ttf"

static int font_size = 16;
static gp_font_face *render_font;
static gp_font_face *render_font_bold;

void gp_widget_render_init(void)
{
	gp_font_face *font = gp_font_face_load(FONT, 0, font_size);
	gp_font_face *font_bold = gp_font_face_load(FONT_BOLD, 0, font_size);

	if (!font || !font_bold) {
		gp_font_face_free(font);
		gp_font_face_free(font_bold);
		return;
	}

	gp_font_face_free(render_font);
	gp_font_face_free(render_font_bold);

	render_font = font;
	render_font_bold = font_bold;

	gp_widget_render_info_.font->font = font;
	gp_widget_render_info_.font_bold->font = font_bold;

	gp_widget_render_info_.font_bold->pixel_xmul = 1;
	gp_widget_render_info_.font_bold->pixel_ymul = 1;
	gp_widget_render_info_.font_bold->pixel_xspace = 0;
	gp_widget_render_info_.font_bold->pixel_yspace = 0;

	gp_widget_render_info_.padd = 2 * gp_text_descent(gp_widget_render_info_.font);
}

static gp_backend *backend;
static gp_widget *app_layout;

void gp_widget_render_zoom(int zoom_inc)
{
	if (font_size + zoom_inc < 5)
		return;

	font_size += zoom_inc;

	gp_widget_render_init();
	gp_widget_resize(app_layout);
	gp_widget_redraw(app_layout);
}

static void timer_event(gp_event *ev)
{
	struct gp_widget *widget = ev->val.tmr->priv;

	gp_widget_ops_event(widget, ev);

	ev->val.tmr->priv = NULL;
}

void gp_widget_render_timer(gp_widget *self, int flags, unsigned int timeout_ms)
{
	static struct gp_timer timers[10];
	unsigned int i;

	for (i = 0; i < GP_ARRAY_SIZE(timers); i++) {
		if (timers[i].priv == self) {
			if (flags & GP_TIMER_RESCHEDULE) {
				gp_backend_rem_timer(backend, &timers[i]);
				timers[i].expires = timeout_ms;
				gp_backend_add_timer(backend, &timers[i]);
				return;
			}

			GP_WARN("Timer for widget %p (%s) allready running!",
			        self, gp_widget_type_id(self));
			return;
		}

		if (!timers[i].priv)
			break;
	}

	if (i >= GP_ARRAY_SIZE(timers)) {
		GP_WARN("All %zu timers used!", GP_ARRAY_SIZE(timers));
		gp_timer_queue_dump(backend->timers);
	}

	timers[i].expires = timeout_ms;
	timers[i].id = gp_widget_type_id(self);
	timers[i].priv = self;

	gp_backend_add_timer(backend, &timers[i]);
}

void gp_widgets_redraw(struct gp_widget *layout)
{
	struct gp_widget_render buf = {backend->pixmap};

	if (!layout) {
		GP_DEBUG(1, "Redraw called with NULL layout!");
		return;
	}

	if (layout->no_redraw && layout->no_redraw_child)
		return;

	if (gp_pixmap_w(backend->pixmap) < layout->w ||
	    gp_pixmap_h(backend->pixmap) < layout->h) {
		gp_backend_resize(backend, layout->w, layout->h);
	}

	gp_widget_render(layout, &buf, 0);
	gp_backend_flip(backend);
}

static uint32_t timer_callback(gp_timer *self)
{
	gp_widget_timer *tmr = self->priv;
	uint32_t expires = tmr->callback(tmr->priv);

	gp_widgets_redraw(app_layout);

	return expires;
}

void gp_widgets_timer_add(struct gp_widget_timer *tmr, uint32_t expires)
{
	tmr->tmr.priv = tmr;
	tmr->tmr.callback = timer_callback;
	tmr->tmr.expires = expires;
	tmr->tmr.period = 0;
	tmr->tmr.id = "App Timer";

	gp_backend_add_timer(backend, &tmr->tmr);
}

static struct gp_widget_render buf;

void gp_widgets_layout_init(gp_widget *layout, const char *win_tittle)
{
	gp_widget_render_init();
	gp_widget_calc_size(layout, 0, 0, 1);

	backend = gp_x11_init(NULL, 0, 0, layout->w, layout->h, win_tittle, 0);
	if (!backend)
		exit(1);

	buf.buf = backend->pixmap;

	app_layout = layout;

	gp_widget_render(layout, &buf, 0);
	gp_backend_flip(backend);
}

int gp_widgets_event(gp_event *ev, gp_widget *layout)
{
	int handled = 0;

	switch (ev->type) {
	case GP_EV_KEY:
		if (gp_event_get_key(ev, GP_KEY_LEFT_CTRL) &&
		    ev->code == GP_EV_KEY_DOWN) {
			switch (ev->val.val) {
			case GP_KEY_UP:
				gp_widget_render_zoom(1);
				handled = 1;
			break;
			case GP_KEY_DOWN:
				gp_widget_render_zoom(-1);
				handled = 1;
			break;
			}
		}
		switch (ev->val.val) {
		case GP_KEY_ESC:
			return 1;
		break;
		}
	break;
	case GP_EV_SYS:
		switch (ev->code) {
		case GP_EV_SYS_RESIZE:
			gp_backend_resize_ack(backend);
			buf.buf = backend->pixmap;
			gp_fill(backend->pixmap, 0x444444);
			gp_widget_render(layout, &buf, 1);
			handled = 1;
		break;
		case GP_EV_SYS_QUIT:
			return 1;
		break;
		}
	break;
	case GP_EV_TMR:
		timer_event(ev);
		handled = 1;
	break;
	}

	if (!handled)
		gp_widget_input_event(layout, ev);

	gp_widgets_redraw(layout);

	return 0;
}

void gp_widgets_main_loop(struct gp_widget *layout, const char *label,
                        void (*init)(void))
{
	gp_widgets_layout_init(layout, label);

	if (init)
		init();

	for (;;) {
		gp_event ev;

		gp_backend_wait_event(backend, &ev);

		if (gp_widgets_event(&ev, layout)) {
			gp_backend_exit(backend);
			return;
		}
	}
}

const struct gp_widget_render_info *cfg = &gp_widget_render_info_;

static gp_size text_size(gp_size add_size, const gp_text_style *style,
                         const char *str, size_t len)
{
	return add_size + gp_text_width_len(style, str, len);
}

static size_t max_chars(const gp_text_style *style, gp_size w, const char *str)
{
	gp_size dots_size = gp_text_width(style, "...");
	size_t left = 0, right = strlen(str);

	for (;;) {
		size_t mid = (left + right)/2;

		if (w > text_size(dots_size, style, str, mid))
			left = mid;
		else
			right = mid;

		if (left >= right - 1) {
			if (text_size(dots_size, style, str, right) <= w)
				return right;
			else
				return left;
		}
	}
}

void gp_text_fit(gp_pixmap *pix, const gp_text_style *style,
                gp_coord x, gp_coord y, gp_size w, int align,
		gp_pixel fg_color, gp_pixel bg_color, const char *str)
{
	gp_size text_w = gp_text_width(style, str);

	if (text_w > w) {
		int chars = max_chars(style, w, str);

		gp_print(pix, style, x, y, align, fg_color, bg_color,
			 "%.*s...", chars, str);
		//gp_hline_xyw(pix, x, y, w, 0xff0000);
		return;
	}

	gp_text(pix, style, x, y, align, fg_color, bg_color, str);
}

void gp_rrect_xywh(gp_pixmap *pix, gp_coord x, gp_coord y,
                   gp_size w, gp_size h, gp_pixel color)
{
	unsigned int rs = 3;

	gp_coord lx = x + rs;
	gp_coord rx = x + w - rs - 1;
	gp_coord uy = y + rs;
	gp_coord dy = y + h - rs - 1;

	gp_circle_seg(pix, lx, uy, rs, GP_CIRCLE_SEG2, color);
	gp_circle_seg(pix, rx, uy, rs, GP_CIRCLE_SEG1, color);
	gp_circle_seg(pix, lx, dy, rs, GP_CIRCLE_SEG3, color);
	gp_circle_seg(pix, rx, dy, rs, GP_CIRCLE_SEG4, color);

	gp_hline_xxy(pix, lx, rx, y, color);
	gp_hline_xxy(pix, lx, rx, y+h-1, color);
	gp_vline_xyy(pix, x, uy, dy, color);
	gp_vline_xyy(pix, x+w-1, uy, dy, color);
}

void gp_fill_rrect_xywh(gp_pixmap *pix, gp_coord x, gp_coord y, gp_size w, gp_size h,
                        gp_pixel bg_color, gp_pixel fg_color, gp_pixel fr_color)
{
	unsigned int rs = 3;

	gp_coord lx = x + rs;
	gp_coord rx = x + w - rs - 1;
	gp_coord uy = y + rs;
	gp_coord dy = y + h - rs - 1;

	gp_fill_rect_xyxy(pix, x, y, lx, uy, bg_color);
	gp_fill_rect_xyxy(pix, rx, y, x+w-1, uy, bg_color);
	gp_fill_rect_xyxy(pix, x, dy, lx, y+h-1, bg_color);
	gp_fill_rect_xyxy(pix, rx, dy, x+w-1, y+h-1, bg_color);

	gp_fill_circle_seg(pix, lx, uy, rs, GP_CIRCLE_SEG2, fg_color);
	gp_fill_circle_seg(pix, rx, uy, rs, GP_CIRCLE_SEG1, fg_color);
	gp_fill_circle_seg(pix, lx, dy, rs, GP_CIRCLE_SEG3, fg_color);
	gp_fill_circle_seg(pix, rx, dy, rs, GP_CIRCLE_SEG4, fg_color);

	gp_fill_rect_xyxy(pix, lx, y+1, rx, y+h-2, fg_color);
	gp_fill_rect_xyxy(pix, x+1, uy, lx-1, dy, fg_color);
	gp_fill_rect_xyxy(pix, rx+1, uy, x+w-2, dy, fg_color);

	gp_circle_seg(pix, lx, uy, rs, GP_CIRCLE_SEG2, fr_color);
	gp_circle_seg(pix, rx, uy, rs, GP_CIRCLE_SEG1, fr_color);
	gp_circle_seg(pix, lx, dy, rs, GP_CIRCLE_SEG3, fr_color);
	gp_circle_seg(pix, rx, dy, rs, GP_CIRCLE_SEG4, fr_color);

	gp_hline_xxy(pix, lx, rx, y, fr_color);
	gp_hline_xxy(pix, lx, rx, y+h-1, fr_color);
	gp_vline_xyy(pix, x, uy, dy, fr_color);
	gp_vline_xyy(pix, x+w-1, uy, dy, fr_color);
}

void gp_triangle_up(gp_pixmap *pix, gp_coord x_center, gp_coord y_center,
                    gp_size base, gp_pixel color)
{
	gp_fill_triangle(pix, x_center, y_center - base/2,
		         x_center + base/2, y_center + base/2,
			 x_center - base/2, y_center + base/2, color);
}

void gp_triangle_down(gp_pixmap *pix, gp_coord x_center, gp_coord y_center,
                      gp_size base, gp_pixel color)
{
	gp_fill_triangle(pix, x_center, y_center + base/2,
		         x_center + base/2, y_center - base/2,
			 x_center - base/2, y_center - base/2, color);
}

void gp_triangle_updown(gp_pixmap *pix, gp_coord x_center, gp_coord y_center,
                        gp_size base, gp_pixel color)
{
	gp_tetragon(pix, x_center, y_center + base/2,
			 x_center - base/2, y_center,
		         x_center, y_center - base/2,
			 x_center + base/2, y_center, color);
}
