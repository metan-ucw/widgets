//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2020 Cyril Hrubis <metan@ucw.cz>

 */

#include <string.h>
#include <json-c/json.h>

#include <gp_widgets.h>
#include <gp_widget_ops.h>
#include <gp_widget_render.h>
#include <gp_widget_json.h>

static gp_size scroll_min_w(gp_widget *self, const gp_widget_render_ctx *ctx)
{
	(void)ctx;
	//TODO: units!
	return self->scroll->min_w;
}

static gp_size scroll_min_h(gp_widget *self, const gp_widget_render_ctx *ctx)
{
	(void)ctx;
	//TODO: units!
	return self->scroll->min_h;
}

static gp_size scrollbar_size(const gp_widget_render_ctx *ctx)
{
	return gp_text_ascent(ctx->font) + ctx->padd;
}

static unsigned int scrolls_x(gp_widget *self, const gp_widget_render_ctx *ctx)
{
	if (!self->scroll->min_w)
		return 0;

	gp_size min_w = gp_widget_min_w(self->scroll->widget, ctx);
	gp_size scroll_w = scroll_min_w(self, ctx);

	if (min_w > scroll_w) {
		self->scroll->scrollbar_y = 1;
		GP_DEBUG(4, "Scroll area %p scrolls horizontally", self);
		return 1;
	}

	self->scroll->scrollbar_y = 0;
	return 0;
}

static unsigned int scrolls_y(gp_widget *self, const gp_widget_render_ctx *ctx)
{
	if (!self->scroll->min_h)
		return 0;

	gp_size min_h = gp_widget_min_h(self->scroll->widget, ctx);
	gp_size scroll_h = scroll_min_h(self, ctx);

	if (min_h > scroll_h) {
		self->scroll->scrollbar_x = 1;
		GP_DEBUG(4, "Scroll area %p scrolls vertically", self);
		return 1;
	}

	self->scroll->scrollbar_x = 0;
	return 0;
}

static unsigned int min_w(gp_widget *self, const gp_widget_render_ctx *ctx)
{
	gp_size min_w;
	gp_size widget_min_w = gp_widget_min_w(self->scroll->widget, ctx);

	if (!self->scroll->min_w)
		min_w = widget_min_w;
	else
		min_w = GP_MIN(scroll_min_w(self, ctx), widget_min_w);

	if (scrolls_y(self, ctx))
		min_w += scrollbar_size(ctx);

	return min_w;
}

static unsigned int min_h(gp_widget *self, const gp_widget_render_ctx *ctx)
{
	gp_size min_h;
	gp_size widget_min_h = gp_widget_min_w(self->scroll->widget, ctx);

	if (!self->scroll->min_h)
		min_h = widget_min_h;
	else
		min_h = GP_MIN(scroll_min_h(self, ctx), widget_min_h);

	if (scrolls_x(self, ctx))
		min_h += scrollbar_size(ctx);

	return min_h;
}

static gp_size scrollbar_w(gp_widget *self, const gp_widget_render_ctx *ctx)
{
	struct gp_widget_scroll_area *area = self->scroll;

	if (area->scrollbar_x && area->scrollbar_y)
		return self->w - scrollbar_size(ctx);

	return self->w;
}

static gp_size scrollbar_h(gp_widget *self, const gp_widget_render_ctx *ctx)
{
	struct gp_widget_scroll_area *area = self->scroll;

	if (area->scrollbar_x && area->scrollbar_y)
		return self->h - scrollbar_size(ctx);

	return self->h;
}

static gp_coord max_x_off(gp_widget *self)
{
	if (self->w > self->scroll->widget->w)
		return 0;

	return self->scroll->widget->w - self->w;
}

static gp_coord max_y_off(gp_widget *self)
{
	if (self->h > self->scroll->widget->h)
		return 0;

	return self->scroll->widget->h - self->h;
}

static void draw_vert_scroll_bar(gp_widget *self, const gp_widget_render_ctx *ctx,
                                 gp_coord x, gp_coord y, gp_size h, gp_size size)
{
	struct gp_widget_scroll_area *area = self->scroll;
	gp_size asc = gp_text_ascent(ctx->font);

	gp_fill_rect_xywh(ctx->buf, x, y, size, h, ctx->bg_color);

	gp_size sh = scrollbar_h(self, ctx);

	gp_vline_xyh(ctx->buf, x + ctx->padd + asc/2, y, sh, ctx->text_color);

	gp_size max_off = max_y_off(self);
	gp_coord pos = ((sh - asc) * area->y_off + max_off/2) / max_off;

	gp_pixel col = area->selected ? ctx->sel_color : ctx->text_color;

	gp_fill_rrect_xywh(ctx->buf, x + ctx->padd, y + pos, asc, asc, ctx->bg_color, ctx->fg_color, col);
}

static void draw_horiz_scroll_bar(gp_widget *self, const gp_widget_render_ctx *ctx,
                                  gp_coord x, gp_coord y, gp_size w, gp_size size)
{
	struct gp_widget_scroll_area *area = self->scroll;
	gp_size asc = gp_text_ascent(ctx->font);

	gp_fill_rect_xywh(ctx->buf, x, y, w, size, ctx->bg_color);

	gp_size sw = scrollbar_w(self, ctx);

	gp_hline_xyw(ctx->buf, x, y + ctx->padd + asc/2, sw, ctx->text_color);

	gp_size max_off = max_x_off(self);
	gp_coord pos = ((sw - asc) * area->x_off + max_off/2) / max_off;

	gp_pixel col = area->selected ? ctx->sel_color : ctx->text_color;

	gp_fill_rrect_xywh(ctx->buf, x + pos, y + ctx->padd, asc, asc, ctx->bg_color, ctx->fg_color, col);
}

static void render(gp_widget *self, const gp_offset *offset,
                   const gp_widget_render_ctx *ctx, int flags)
{
	struct gp_widget_scroll_area *area = self->scroll;
	gp_widget_render_ctx child_ctx = *ctx;
	gp_pixmap child_buf;
	gp_offset child_offset = {
		.x = -area->x_off,
		.y = -area->y_off,
	};

	//TODO!!!
	gp_widget_ops_blit(ctx, self->x + offset->x, self->y + offset->y, self->w, self->h);

	gp_size w = self->w;
	gp_size h = self->h;
	gp_size size = scrollbar_size(ctx);

	if (area->scrollbar_x) {
		w -= size;
		draw_vert_scroll_bar(self, ctx, self->x + offset->x + w, self->y + offset->y, h, size);
	}

	if (area->scrollbar_y) {
		h -= size;
		draw_horiz_scroll_bar(self, ctx, self->x + offset->x, self->y + offset->y + h, w, size);
	}

	gp_sub_pixmap(ctx->buf, &child_buf,
	              offset->x + self->x,
	              offset->y + self->y,
		      w, h);

	//TODO: Combine with passed down bbox?
	gp_bbox child_bbox = gp_bbox_pack(0, 0, w, h);

	child_ctx.bbox = &child_bbox;
	child_ctx.buf = &child_buf;
	//TODO: Propagate flip
	child_ctx.flip = NULL;

	if (self->redraw_subtree) {
		self->redraw_subtree = 0;
		flags |= 1;
	}

	gp_widget_ops_render(area->widget, &child_offset, &child_ctx, flags);
	gp_rect_xywh(ctx->buf, self->x + offset->x, self->y + offset->y, w, h, ctx->text_color);
}

static int is_in_scrollbar_x(gp_widget *self, const gp_widget_render_ctx *ctx, unsigned int x)
{
	struct gp_widget_scroll_area *area = self->scroll;

	if (area->scrollbar_x) {
		if (x > self->x + self->w - scrollbar_size(ctx))
			return 1;
	}

	return 0;
}

static int is_in_scrollbar_y(gp_widget *self, const gp_widget_render_ctx *ctx, unsigned int y)
{
	struct gp_widget_scroll_area *area = self->scroll;

	if (area->scrollbar_y) {
		if (y > self->y + self->h - scrollbar_size(ctx))
			return 1;
	}

	return 0;
}

static void set_y_off(gp_widget *self, int y_off)
{
	if (y_off < 0) {
		GP_WARN("y_off < 0");
		return;
	}

	if (y_off > max_y_off(self)) {
		GP_WARN("y_off > max y_off");
		return;
	}

	if (self->scroll->y_off == y_off)
		return;

	self->scroll->y_off = y_off;

        gp_widget_redraw(self);
	self->redraw_subtree = 1;
}

static void set_x_off(gp_widget *self, int x_off)
{
	if (x_off < 0) {
		GP_WARN("x_off < 0");
		return;
	}

	if (x_off > max_x_off(self)) {
		GP_WARN("y_off > max y_off");
		return;
	}

	if (self->scroll->x_off == x_off)
		return;

	self->scroll->x_off = x_off;

        gp_widget_redraw(self);
	self->redraw_subtree = 1;
}

static void scrollbar_event_y(gp_widget *self, const gp_widget_render_ctx *ctx, gp_event *ev)
{
	gp_size asc = gp_text_ascent(ctx->font);
	gp_size gh = scrollbar_h(self, ctx) - asc;
	gp_coord y = ev->cursor_y - self->y - asc/2;

	if (y < 0)
		y = 0;

	if ((gp_size)y > gh)
		y = gh;

	set_y_off(self, (y * max_y_off(self) + gh/2)/ gh);
}

static void scrollbar_event_x(gp_widget *self, const gp_widget_render_ctx *ctx, gp_event *ev)
{
	gp_size asc = gp_text_ascent(ctx->font);
	gp_size gw = scrollbar_w(self, ctx) - asc;
	gp_coord x = ev->cursor_x - self->x - asc/2;

	if (x < 0)
		x = 0;

	if ((gp_size)x > gw)
		x = gw;

	set_x_off(self, (x * max_x_off(self) + gw/2)/ gw);
}

static int event(gp_widget *self, const gp_widget_render_ctx *ctx, gp_event *ev)
{
	struct gp_widget_scroll_area *area = self->scroll;

	if (is_in_scrollbar_y(self, ctx, ev->cursor_y)) {
		if (gp_event_get_key(ev, GP_BTN_LEFT)) {
			scrollbar_event_x(self, ctx, ev);
			return 1;
		}
	}

	if (is_in_scrollbar_x(self, ctx, ev->cursor_x)) {
		if (gp_event_get_key(ev, GP_BTN_LEFT)) {
			scrollbar_event_y(self, ctx, ev);
			return 1;
		}
	}

	if (area->selected) {
		if (ev->type != GP_EV_KEY)
			return 0;

		if (ev->code == GP_EV_KEY_UP)
			return 0;

		switch (ev->val.val) {
		case GP_KEY_LEFT:
			set_x_off(self, GP_MAX(0, area->x_off - 10));
		break;
		case GP_KEY_RIGHT:
			set_x_off(self, GP_MIN(max_x_off(self), area->x_off + 10));
		break;
		case GP_KEY_UP:
			set_y_off(self, GP_MAX(0, area->y_off - 10));
		break;
		case GP_KEY_DOWN:
			set_y_off(self, GP_MIN(max_y_off(self), area->y_off + 10));
		break;
		}

		return 0;
	}


	ev->cursor_x += area->x_off;
	ev->cursor_y += area->y_off;

	int ret = gp_widget_ops_event(area->widget, ctx, ev);

	ev->cursor_x -= area->x_off;
	ev->cursor_y -= area->y_off;

	return ret;
}

static int select_scrollbar(gp_widget *self)
{
	struct gp_widget_scroll_area *area = self->scroll;

	if (area->selected)
		return 1;

	area->selected = 1;
        gp_widget_ops_render_select(area->widget, GP_SELECT_OUT);
	area->widget_selected = 0;

	gp_widget_redraw(self);

	return 1;
}

static void select_out(gp_widget *self)
{
	struct gp_widget_scroll_area *area = self->scroll;

	if (area->selected) {
		area->selected = 0;
		gp_widget_redraw(self);
	}
}

static int select_widget(gp_widget *self, const gp_widget_render_ctx *ctx,
                         unsigned int x, unsigned int y)
{
	struct gp_widget_scroll_area *area = self->scroll;

	if (!gp_widget_ops_render_select_xy(area->widget, ctx, x + area->x_off, y + area->y_off))
		return 0;

	select_out(self);

	area->widget_selected = 1;
	return 1;
}

static int select_xy(gp_widget *self, const gp_widget_render_ctx *ctx,
                     unsigned int x, unsigned int y)
{
	if (is_in_scrollbar_x(self, ctx, x) ||
	    is_in_scrollbar_y(self, ctx, y))
		return select_scrollbar(self);

	return select_widget(self, ctx, x, y);
}

static int select_event(gp_widget *self, int sel)
{
	if (self->scroll->widget_selected) {
		if (gp_widget_ops_render_select(self->scroll->widget, sel))
			return 1;
	}

	switch (sel) {
	case GP_SELECT_OUT:
		select_out(self);
	break;
	}

	return 0;
}

static void distribute_size(gp_widget *self, const gp_widget_render_ctx *ctx, int new_wh)
{
	struct gp_widget_scroll_area *area = self->scroll;

	gp_size child_min_w = gp_widget_min_w(area->widget, ctx);
	gp_size child_min_h = gp_widget_min_h(area->widget, ctx);
	gp_size w = self->w;
	gp_size h = self->h;

	if (area->scrollbar_x)
		w -= scrollbar_size(ctx);

	if (area->scrollbar_y)
		h -= scrollbar_size(ctx);

	gp_size child_w = GP_MAX(child_min_w, w);
	gp_size child_h = GP_MAX(child_min_h, h);

	gp_coord x_off = max_x_off(self);
	gp_coord y_off = max_y_off(self);

	if (area->x_off > x_off)
		area->x_off = x_off;

	if (x_off == 0)
		area->scrollbar_y = 0;
	else
		area->scrollbar_y = 1;

	if (area->y_off > y_off)
		area->y_off = y_off;

	if (y_off == 0)
		area->scrollbar_x = 0;
	else
		area->scrollbar_x = 1;

	gp_widget_ops_distribute_size(area->widget, ctx, child_w, child_h, new_wh);
}

static gp_widget *json_to_scroll(json_object *json, void **uids)
{
	json_object *widget = NULL;
	int min_w = 0;
	int min_h = 0;

        json_object_object_foreach(json, key, val) {
		if (!strcmp(key, "min_w")) {
			min_w = json_object_get_int(val);
		} else if (!strcmp(key, "min_h")) {
			min_h = json_object_get_int(val);
		} else if (!strcmp(key, "widget")) {
			widget = val;
		} else
			GP_WARN("Invalid scroll area key '%s'", key);
	}

	if (min_w < 0 || min_h < 0) {
		GP_WARN("min_w and min_h must be >= 0");
		return NULL;
	}

	if (min_w == 0 && min_h == 0) {
		GP_WARN("At least one of min_w and min_h must be > 0");
		return NULL;
	}

	gp_widget *ret = gp_widget_scroll_area_new(min_w, min_h);

	if (widget) {
		ret->scroll->widget = gp_widget_from_json(widget, uids);
		if (ret->scroll->widget)
			ret->scroll->widget->parent = ret;
	}

	return ret;
}

struct gp_widget_ops gp_widget_scroll_area_ops = {
	.min_w = min_w,
	.min_h = min_h,
	.render = render,
	.event = event,
	.select_xy = select_xy,
	.select = select_event,
	.distribute_size = distribute_size,
	.from_json = json_to_scroll,
	.id = "scroll area",
};

gp_widget *gp_widget_scroll_area_new(gp_size min_w, gp_size min_h)
{
	gp_widget *ret;

	ret = gp_widget_new(GP_WIDGET_SCROLL_AREA, sizeof(struct gp_widget_scroll_area));
	if (!ret)
		return NULL;

	ret->scroll->min_w = min_w;
	ret->scroll->min_h = min_h;

	return ret;
}

static int move_x(gp_widget *self, gp_coord x_off)
{
	struct gp_widget_scroll_area *area = self->scroll;

	gp_coord old_x_off = area->x_off;

	area->x_off += x_off;

	if (area->x_off < 0)
		area->x_off = 0;

	if (area->x_off + self->w > area->widget->w)
		area->x_off = area->widget->w - self->w;

	if (area->x_off == old_x_off)
		return 0;

	return 1;
}

static int move_y(gp_widget *self, gp_coord y_off)
{
	struct gp_widget_scroll_area *area = self->scroll;

	gp_coord old_y_off = area->y_off;

	area->y_off += y_off;

	if (area->y_off < 0)
		area->y_off = 0;

	if (area->y_off + self->h > area->widget->h)
		area->y_off = area->widget->h - self->h;

	if (area->y_off == old_y_off)
		return 0;

	return 1;
}

int gp_widget_scroll_area_move(gp_widget *self, gp_coord x_off, gp_coord y_off)
{
	GP_WIDGET_ASSERT(self, GP_WIDGET_SCROLL_AREA, 1);

	struct gp_widget_scroll_area *area = self->scroll;

	if (!area->widget)
		return 1;

	int ret = 0;

	if (area->scrollbar_y)
		ret |= move_x(self, x_off);

	if (area->scrollbar_x)
		ret |= move_y(self, y_off);

	if (!ret)
		return ret;

	gp_widget_redraw(self);
	self->redraw_subtree = 1;

	return 1;
}

gp_widget *gp_widget_scroll_area_put(gp_widget *self, gp_widget *widget)
{
	GP_WIDGET_ASSERT(self, GP_WIDGET_SCROLL_AREA, NULL);

	gp_widget *ret = self->scroll->widget;

	self->scroll->widget = widget;
	widget->parent = self;

	gp_widget_resize(self);

	return ret;
}
