//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2020 Cyril Hrubis <metan@ucw.cz>

 */

#include <errno.h>
#include <string.h>

#include <core/gp_debug.h>
#include <core/gp_common.h>
#include <utils/gp_fds.h>
#include <backends/gp_backends.h>
#include <input/gp_input_driver_linux.h>
#include <gp_widget_render.h>
#include <gp_widget_ops.h>

static struct gp_text_style font = {
	.pixel_xmul = 1,
	.pixel_ymul = 1,
	.font = &gp_default_font,
};

static struct gp_text_style font_bold = {
	.pixel_xmul = 1,
	.pixel_ymul = 1,
	.font = &gp_default_font,
};

static struct gp_widget_render_ctx ctx = {
	.text_color = 0,
	.font = &font,
	.font_bold = &font_bold,
	.padd = 4,
};

static gp_pixel fill_color;

static int font_size = 16;
static gp_font_face *render_font;
static gp_font_face *render_font_bold;

const char *arg_fonts;
const char *input_str;

static void set_default_font(void)
{
	/* Embolding */
	font_bold.pixel_xmul = 2;
	font_bold.pixel_ymul = 2;
	font_bold.pixel_xspace = -1;
	font_bold.pixel_yspace = -1;
}

static void init_fonts(void)
{
	if (arg_fonts) {
		if (!strcmp(arg_fonts, "default")) {
			set_default_font();
			return;
		}

		if (!strcmp(arg_fonts, "haxor-15")) {
			font.font = gp_font_haxor_narrow_15;
			font_bold.font = gp_font_haxor_narrow_bold_15;
			return;
		}

		if (!strcmp(arg_fonts, "haxor-16")) {
			font.font = gp_font_haxor_narrow_16;
			font_bold.font = gp_font_haxor_narrow_bold_16;
			return;
		}

		if (!strcmp(arg_fonts, "haxor-17")) {
			font.font = gp_font_haxor_narrow_17;
			font_bold.font = gp_font_haxor_narrow_bold_17;
			return;
		}

		GP_WARN("Inavlid font '%s'!", arg_fonts);
		return;
	}

	gp_font_face *ffont = gp_font_face_fc_load("DroidSans", 0, font_size);
	gp_font_face *ffont_bold = gp_font_face_fc_load("DroidSans:Bold", 0, font_size);

	if (!ffont || !ffont_bold) {
		gp_font_face_free(ffont);
		gp_font_face_free(ffont_bold);
		set_default_font();
		return;
	}

	gp_font_face_free(render_font);
	gp_font_face_free(render_font_bold);

	render_font = ffont;
	render_font_bold = ffont_bold;

	ctx.font->font = ffont;
	ctx.font_bold->font = ffont_bold;
}

void gp_widget_render_init(void)
{
	init_fonts();

	ctx.padd = 2 * gp_text_descent(ctx.font);
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

	gp_widget_ops_event(widget, &ctx, ev);

	ev->val.tmr->priv = NULL;
}

static struct gp_timer timers[10];

void gp_widget_render_timer(gp_widget *self, int flags, unsigned int timeout_ms)
{
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

void gp_widget_render_timer_cancel(gp_widget *self)
{
	unsigned int i;

	for (i = 0; i < GP_ARRAY_SIZE(timers); i++) {
		if (timers[i].priv == self) {
			gp_backend_rem_timer(backend, &timers[i]);
			timers[i].priv = NULL;
			return;
		}

		if (!timers[i].priv)
			break;
	}
}

void gp_widgets_redraw(struct gp_widget *layout)
{
	if (!layout) {
		GP_DEBUG(1, "Redraw called with NULL layout!");
		return;
	}

	if (!layout->redraw && !layout->redraw_child)
		return;

	if (gp_pixmap_w(backend->pixmap) < layout->w ||
	    gp_pixmap_h(backend->pixmap) < layout->h) {
		gp_backend_resize(backend, layout->w, layout->h);
	}

	if (gp_pixmap_w(backend->pixmap) == 0 ||
	    gp_pixmap_h(backend->pixmap) == 0)
		return;

	gp_bbox flip = {};

	ctx.flip = &flip;
	gp_widget_render(layout, &ctx, 0);
	ctx.flip = NULL;

	if (gp_bbox_empty(flip))
		return;

	GP_DEBUG(1, "Updating area " GP_BBOX_FMT, GP_BBOX_PARS(flip));

	gp_backend_update_rect_xywh(backend, flip.x, flip.y, flip.w, flip.h);
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

static char *backend_init_str = "x11";

void gp_widgets_layout_init(gp_widget *layout, const char *win_tittle)
{
	gp_widget_render_init();

	backend = gp_backend_init(backend_init_str, win_tittle);
	if (!backend)
		exit(1);

	ctx.buf = backend->pixmap;
	ctx.pixel_type = backend->pixmap->pixel_type;

	ctx.bg_color = gp_rgb_to_pixmap_pixel(0xdd, 0xdd, 0xdd, ctx.buf);
	ctx.fg_color = gp_rgb_to_pixmap_pixel(0xee, 0xee, 0xee, ctx.buf);
	ctx.fg2_color = gp_rgb_to_pixmap_pixel(0x77, 0xbb, 0xff, ctx.buf);
	ctx.sel_color = gp_rgb_to_pixmap_pixel(0x11, 0x99, 0xff, ctx.buf);
	ctx.alert_color = gp_rgb_to_pixmap_pixel(0xff, 0x55, 0x55, ctx.buf);
	fill_color = gp_rgb_to_pixmap_pixel(0x44, 0x44, 0x44, ctx.buf);

	gp_widget_calc_size(layout, &ctx, 0, 0, 1);

	gp_backend_resize(backend, layout->w, layout->h);

	app_layout = layout;

	int flag = 0;

	if (layout->w != gp_pixmap_w(backend->pixmap) ||
	    layout->h != gp_pixmap_h(backend->pixmap)) {
		gp_fill(backend->pixmap, fill_color);
		flag = 1;
	}

	if (gp_pixmap_w(backend->pixmap) == 0 ||
	    gp_pixmap_h(backend->pixmap) == 0)
		return;

	gp_widget_render(layout, &ctx, flag);
	gp_backend_flip(backend);
}

static int (*app_event_callback)(gp_event *);

void gp_widgets_register_callback(int (*event_callback)(gp_event *))
{
	app_event_callback = event_callback;
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
		if ((gp_event_get_key(ev, GP_KEY_LEFT_ALT) ||
		     gp_event_get_key(ev, GP_KEY_LEFT_ALT)) &&
		     ev->code == GP_EV_KEY_DOWN) {
			switch (ev->val.val) {
			case GP_KEY_F4:
				return 1;
			}
		}
	break;
	case GP_EV_SYS:
		switch (ev->code) {
		case GP_EV_SYS_RESIZE:
			gp_backend_resize_ack(backend);
			ctx.buf = backend->pixmap;
			gp_fill(backend->pixmap, fill_color);
			gp_widget_render(layout, &ctx, 1);
			gp_backend_flip(backend);
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

	if (handled)
		return 0;

	handled = gp_widget_input_event(layout, &ctx, ev);

	if (handled)
		return 0;

	if (app_event_callback)
		app_event_callback(ev);

	return 0;
}

int gp_widgets_process_events(gp_widget *layout)
{
	gp_event ev;

	while (gp_backend_poll_event(backend, &ev)) {
		gp_event_dump(&ev);
		fflush(stdout);
		if (gp_widgets_event(&ev, layout)) {
			gp_backend_exit(backend);
			exit(0);
		}
	}

	return 0;
}

int gp_widgets_fd(void)
{
	return backend->fd;
}

static void print_options(int exit_val)
{
	printf("Options:\n--------\n");
	printf("\t-b backend init string (pass -b help for options)\n");
	printf("\t-f fonts\n\t\tdefault\n\t\thaxor-15\n\t\thaxor-16\n\t\thaxor-17\n");
	printf("\t-i input_string\n");
	exit(exit_val);
}

void gp_widgets_getopt(int *argc, char **argv[])
{
	int opt;

	while ((opt = getopt(*argc, *argv, "b:f:hi:")) != -1) {
		switch (opt) {
		case 'b':
			backend_init_str = optarg;
		break;
		case 'h':
			print_options(0);
		break;
		case 'f':
			arg_fonts = optarg;
		break;
		case 'i':
			input_str = optarg;
		break;
		default:
			print_options(1);
		}
	}

	*argv = *argv + optind;
	*argc -= optind;
}

static gp_widget *win_layout;

gp_widget *gp_widget_layout_replace(gp_widget *layout)
{
	gp_widget *ret = win_layout;

	gp_widget_resize(layout);
	gp_widget_redraw(layout);

	win_layout = layout;

	return ret;
}

static int backend_event(struct gp_fd *self, struct pollfd *pfd)
{
	(void) pfd;
	(void) self;

	if (gp_widgets_process_events(win_layout))
		return 1;

	return 0;
}

static int input_event(struct gp_fd *self, struct pollfd *pfd)
{
	while (gp_input_linux_read(self->priv, &backend->event_queue) > 0);

	if (gp_widgets_process_events(win_layout))
		return 1;

	return 0;
}

static struct gp_fds fds = GP_FDS_INIT;

struct gp_fds *gp_widgets_fds = &fds;

void gp_widgets_main_loop(gp_widget *layout, const char *label,
                          void (*init)(void), int argc, char *argv[])
{
	if (argv)
		gp_widgets_getopt(&argc, &argv);

	gp_widgets_layout_init(layout, label);

	win_layout = layout;

	if (init)
		init();

	gp_fds_add(&fds, backend->fd, POLLIN, backend_event, NULL);

	if (input_str) {
		gp_input_linux *input = gp_input_linux_by_devstr(input_str);

		if (input)
			gp_fds_add(&fds, input->fd, POLLIN, input_event, input);
	}

	for (;;) {
		int ret = gp_fds_poll(&fds, gp_backend_timer_timeout(backend));
		/* handle timers */
		if (ret == 0) {
			gp_backend_poll(backend);
			gp_widgets_process_events(win_layout);
		}
		gp_widgets_redraw(win_layout);
	}
}

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
