//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2020 Cyril Hrubis <metan@ucw.cz>

 */

#include <string.h>
#include <json-c/json.h>

#include <utils/gp_vec.h>

#include <gp_widgets.h>
#include <gp_widget_ops.h>
#include <gp_widget_render.h>

static unsigned int min_w(gp_widget *self, const gp_widget_render_ctx *ctx)
{
	unsigned int i, max_w = 0;
	struct gp_widget_overlay *o = self->overlay;

	for (i = 0; i < gp_widget_overlay_widgets(self); i++)
		max_w = GP_MAX(max_w, gp_widget_min_w(o->stack[i].widget, ctx));

	return max_w;
}

static unsigned int min_h(gp_widget *self, const gp_widget_render_ctx *ctx)
{
	unsigned int i, max_h = 0;
	struct gp_widget_overlay *o = self->overlay;

	for (i = 0; i < gp_widget_overlay_widgets(self); i++)
		max_h = GP_MAX(max_h, gp_widget_min_h(o->stack[i].widget, ctx));

	return max_h;
}

static void distribute_size(gp_widget *self, const gp_widget_render_ctx *ctx,
                            int new_wh)
{
	unsigned int i;
	struct gp_widget_overlay *o = self->overlay;

	for (i = 0; i < gp_widget_overlay_widgets(self); i++) {
		gp_widget *widget = o->stack[i].widget;

		if (!widget)
			continue;

		gp_widget_ops_distribute_size(widget, ctx, self->w, self->h, new_wh);
	}
}

static gp_widget *get_selected_widget(gp_widget *self)
{
	if (self->overlay->selected < 0)
		return NULL;

	return self->overlay->stack[self->overlay->selected].widget;
}

static int event(gp_widget *self, const gp_widget_render_ctx *ctx, gp_event *ev)
{
	gp_widget *widget = get_selected_widget(self);

	return gp_widget_ops_event_offset(widget, ctx, ev, 0, 0);
}

static void render(gp_widget *self, const gp_offset *offset,
                   const gp_widget_render_ctx *ctx, int flags)
{
	unsigned int x = self->x + offset->x;
	unsigned int y = self->y + offset->y;
	unsigned int w = self->w;
	unsigned int h = self->h;
	unsigned int i;

	gp_widget_ops_blit(ctx, x, y, w, h);

	//TODO!
	//gp_fill_rect_xywh(ctx->buf, self->x, self->y, self->w, self->h, ctx->bg_color);

	gp_offset child_offset = {
		.x = x,
		.y = y,
	};

	for (i = 0; i < gp_widget_overlay_widgets(self); i++) {
		gp_widget *widget = self->overlay->stack[i].widget;

		if (self->overlay->stack[i].hidden)
			continue;

		gp_widget_ops_render(widget, &child_offset, ctx, flags);
	}
}

static gp_widget *json_to_overlay(json_object *json, void **uids)
{
	json_object *widgets = NULL;

	json_object_object_foreach(json, key, val) {
		if (!strcmp(key, "widgets"))
			widgets = val;
		else
			GP_WARN("Invalid overlay key '%s'", key);
	}

	if (!widgets) {
		GP_WARN("Missing widgets array!");
		return NULL;
	}

	if (!json_object_is_type(widgets, json_type_array)) {
		GP_WARN("Widgets has to be array of strings!");
		return NULL;
	}

	unsigned int i, stack_size = json_object_array_length(widgets);

	gp_widget *ret = gp_widget_overlay_new(stack_size);
	if (!ret)
		return NULL;

	for (i = 0; i < stack_size; i++) {
		json_object *json_widget = json_object_array_get_idx(widgets, i);

		ret->overlay->stack[i].widget = gp_widget_from_json(json_widget, uids);

		gp_widget_set_parent(ret->switch_->layouts[i], ret);
	}

	return ret;
}

static gp_widget *select_widget_by_xy(gp_widget *self, unsigned int x, unsigned int y)
{
	int i;

	for (i = gp_widget_overlay_widgets(self) - 1; i > 0; i--) {
		gp_widget *widget = self->overlay->stack[i].widget;

		if (!widget)
			continue;

		if (x >= widget->x && y >= widget->y &&
		    x < widget->x + widget->w &&
		    y < widget->y + widget->w) {
			self->overlay->selected = i;
			return widget;
		}
	}

	return NULL;
}

static void free_(gp_widget *self)
{
	unsigned int i;
	struct gp_widget_overlay *o = self->overlay;

	for (i = 0; i < gp_widget_overlay_widgets(self); i++)
		gp_widget_free(o->stack[i].widget);

	gp_vec_free(o->stack);
}

static int select_ev(gp_widget *self, int sel)
{
	int i;

	if (self->overlay->selected < 0) {
		for (i = gp_widget_overlay_widgets(self) - 1; i > 0; i--) {
			gp_widget *widget = self->overlay->stack[i].widget;

			if (self->overlay->stack[i].hidden)
				continue;

			if (gp_widget_ops_render_select(widget, sel)) {
				self->overlay->selected = i;
				return 1;
			}

		}

		return 0;
	}

	return gp_widget_ops_render_select(get_selected_widget(self), sel);
}

static int select_xy(gp_widget *self, const gp_widget_render_ctx *ctx,
                     unsigned int x, unsigned int y)
{
	int ret;
	gp_widget *widget = select_widget_by_xy(self, x, y);

	if (!widget)
		return 0;

	x -= self->x;
	y -= self->y;
	ret = gp_widget_ops_render_select_xy(widget, ctx, x, y);
	x += self->x;
	y += self->y;

	return ret;
}

struct gp_widget_ops gp_widget_overlay_ops = {
	.min_w = min_w,
	.min_h = min_h,
	.distribute_size = distribute_size,
	.event = event,
	.select = select_ev,
	.select_xy = select_xy,
	.free = free_,
	.render = render,
	.from_json = json_to_overlay,
	.id = "overlay",
};

gp_widget *gp_widget_overlay_new(unsigned int stack_size)
{
	gp_widget *ret;

	ret = gp_widget_new(GP_WIDGET_OVERLAY, sizeof(struct gp_widget_overlay));
	if (!ret)
		return NULL;

	ret->overlay->stack = gp_vec_new(stack_size, sizeof(struct gp_widget_overlay_elem));

	if (!ret->overlay->stack) {
		free(ret);
		return NULL;
	}

	ret->overlay->selected = -1;

	return ret;
}

unsigned int gp_widget_overlay_widgets(gp_widget *self)
{
	GP_WIDGET_ASSERT(self, GP_WIDGET_OVERLAY, 0);

	return gp_vec_len(self->overlay->stack);
}

static int stack_pos_is_invalid(gp_widget *self, unsigned int stack_pos)
{
	if (stack_pos >= gp_widget_overlay_widgets(self)) {
		GP_WARN("Invalid stack_pos %u", stack_pos);
		return 1;
	}

	return 0;
}

void gp_widget_overlay_hide(gp_widget *self, unsigned int stack_pos)
{
	struct gp_widget_overlay *o = self->overlay;

	GP_WIDGET_ASSERT(self, GP_WIDGET_OVERLAY, );

	if (stack_pos_is_invalid(self, stack_pos))
		return;

	if (o->stack[stack_pos].hidden)
		return;

	o->stack[stack_pos].hidden = 1;

	gp_widget_redraw_children(self);
}

void gp_widget_overlay_show(gp_widget *self, unsigned int stack_pos)
{
	struct gp_widget_overlay *o = self->overlay;

	GP_WIDGET_ASSERT(self, GP_WIDGET_OVERLAY, );

	if (stack_pos_is_invalid(self, stack_pos))
		return;

	if (!o->stack[stack_pos].hidden)
		return;

	o->stack[stack_pos].hidden = 0;

	gp_widget_redraw_children(self);
}

gp_widget *gp_widget_overlay_set(gp_widget *self, unsigned int stack_pos,
                                 gp_widget *widget)
{
	gp_widget *ret;

	GP_WIDGET_ASSERT(self, GP_WIDGET_OVERLAY, NULL);

	if (stack_pos_is_invalid(self, stack_pos))
		return NULL;

	ret = self->overlay->stack[stack_pos].widget;
	self->overlay->stack[stack_pos].widget = widget;

	gp_widget_set_parent(widget, self);

	gp_widget_resize(self);

	return ret;
}
