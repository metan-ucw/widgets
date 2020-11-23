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
	struct gp_widget_switch *s = self->switch_;

	for (i = 0; i < gp_widget_switch_layouts(self); i++)
		max_w = GP_MAX(max_w, gp_widget_min_w(s->layouts[i], ctx));

	return max_w;
}

static unsigned int min_h(gp_widget *self, const gp_widget_render_ctx *ctx)
{
	unsigned int i, max_h = 0;
	struct gp_widget_switch *s = self->switch_;

	for (i = 0; i < gp_widget_switch_layouts(self); i++)
		max_h = GP_MAX(max_h, gp_widget_min_h(s->layouts[i], ctx));

	return max_h;
}

static void distribute_size(gp_widget *self, const gp_widget_render_ctx *ctx,
                            int new_wh)
{
	unsigned int i;
	struct gp_widget_switch *s = self->switch_;

	for (i = 0; i < gp_widget_switch_layouts(self); i++) {
		gp_widget *widget = s->layouts[i];

		if (!widget)
			continue;

		gp_widget_ops_distribute_size(widget, ctx, self->w, self->h, new_wh);
	}
}

static int event(gp_widget *self, const gp_widget_render_ctx *ctx, gp_event *ev)
{
	struct gp_widget_switch *s = self->switch_;
	gp_widget *widget = s->layouts[s->active_layout];

	if (!widget)
		return 0;

	return gp_widget_ops_event(widget, ctx, ev);
}

static void render(gp_widget *self, const gp_offset *offset,
                   const gp_widget_render_ctx *ctx, int flags)
{
	unsigned int x = self->x + offset->x;
	unsigned int y = self->y + offset->y;
	unsigned int w = self->w;
	unsigned int h = self->h;
	gp_widget *layout = gp_widget_switch_active(self);

	gp_widget_ops_blit(ctx, x, y, w, h);

	if (!layout) {
		gp_fill_rect_xywh(ctx->buf, self->x, self->y, self->w, self->h, ctx->bg_color);
		return;
	}

	gp_offset child_offset = {
		.x = x,
		.y = y,
	};

	gp_fill_rect_xywh(ctx->buf, x, y, layout->x, self->h, ctx->bg_color);
	gp_fill_rect_xywh(ctx->buf, x + layout->x + layout->w, y,
	                  self->w - layout->x - layout->w, self->h, ctx->bg_color);
	gp_fill_rect_xywh(ctx->buf, x + layout->x, y, layout->w, layout->y, ctx->bg_color);
	gp_fill_rect_xywh(ctx->buf, x + layout->x, y + layout->y + layout->h,
	                  layout->w, self->h - layout->y - layout->h, ctx->bg_color);

	if (self->redraw_subtree) {
		flags |= 1;
		self->redraw_subtree = 0;
	}

	gp_widget_ops_render(layout, &child_offset, ctx, flags);
}

static gp_widget *json_to_switch(json_object *json, void **uids)
{
	const char *label = NULL;
	int bold = 0;
	int size = 0;
	int ralign = 0;
	int frame = 0;

	(void)uids;

	json_object_object_foreach(json, key, val) {
		if (!strcmp(key, "text"))
			label = json_object_get_string(val);
		else if (!strcmp(key, "bold"))
			bold = json_object_get_boolean(val);
		else if (!strcmp(key, "size"))
			size = json_object_get_int(val);
		else if (!strcmp(key, "ralign"))
			ralign = json_object_get_boolean(val);
		else if (!strcmp(key, "frame"))
			frame = json_object_get_boolean(val);
		else
			GP_WARN("Invalid label key '%s'", key);
	}

	if (!label) {
		GP_WARN("Missing label");
		label = "Missing label";
	}

	gp_widget *ret = gp_widget_label_new(label, size, bold);

	ret->label->ralign = ralign;
	ret->label->frame = frame;

	return ret;
}

static void free_(gp_widget *self)
{
	unsigned int i;
	struct gp_widget_switch *s = self->switch_;

	for (i = 0; i < gp_widget_switch_layouts(self); i++)
		gp_widget_free(s->layouts[i]);

	gp_vec_free(s->layouts);
}

static int select_ev(gp_widget *self, int sel)
{
	return gp_widget_ops_render_select(gp_widget_switch_active(self), sel);
}

static int select_xy(gp_widget *self, const gp_widget_render_ctx *ctx,
                     unsigned int x, unsigned int y)
{
	return gp_widget_ops_render_select_xy(gp_widget_switch_active(self), ctx, x, y);
}

struct gp_widget_ops gp_widget_switch_ops = {
	.min_w = min_w,
	.min_h = min_h,
	.distribute_size = distribute_size,
	.event = event,
	.select = select_ev,
	.select_xy = select_xy,
	.free = free_,
	.render = render,
	.from_json = json_to_switch,
	.id = "switch",
};

gp_widget *gp_widget_switch_new(unsigned int layouts)
{
	gp_widget *ret;

	ret = gp_widget_new(GP_WIDGET_SWITCH, sizeof(struct gp_widget_switch));
	if (!ret)
		return NULL;

	ret->switch_->active_layout = 0;
	ret->switch_->layouts = gp_vec_new(layouts, sizeof(gp_widget*));

	if (!ret->switch_->layouts) {
		free(ret);
		return NULL;
	}

	return ret;
}

gp_widget *gp_widget_switch_active(gp_widget *self)
{
	GP_WIDGET_ASSERT(self, GP_WIDGET_SWITCH, NULL);

	return self->switch_->layouts[self->switch_->active_layout];
}

unsigned int gp_widget_switch_layouts(gp_widget *self)
{
	GP_WIDGET_ASSERT(self, GP_WIDGET_SWITCH, 0);

	return gp_vec_len(self->switch_->layouts);
}

void gp_widget_switch_move(gp_widget *self, int where)
{
	GP_WIDGET_ASSERT(self, GP_WIDGET_SWITCH, );
	int layouts = gp_vec_len(self->switch_->layouts);

	int switch_to = ((int)self->switch_->active_layout + where) % layouts;

	if (switch_to < 0)
		switch_to += layouts;

	gp_widget_switch_layout(self, switch_to);
}

gp_widget *gp_widget_switch_set(gp_widget *self, unsigned int layout_nr,
                                gp_widget *layout)
{
	gp_widget *ret;

	GP_WIDGET_ASSERT(self, GP_WIDGET_SWITCH, NULL);

	if (layout_nr >= gp_widget_switch_layouts(self))
		return NULL;

	ret = self->switch_->layouts[layout_nr];
	self->switch_->layouts[layout_nr] = layout;

	gp_widget_set_parent(layout, self);

	gp_widget_resize(self);

	return ret;
}

void gp_widget_switch_layout(gp_widget *self, unsigned int layout_nr)
{
	struct gp_widget_switch *s = self->switch_;

	GP_WIDGET_ASSERT(self, GP_WIDGET_SWITCH, );

	if (layout_nr >= gp_widget_switch_layouts(self)) {
		GP_WARN("Invalid layout nr %i", layout_nr);
		return;
	}

	s->active_layout = layout_nr;

	self->redraw_subtree = 1;
	gp_widget_redraw(self);
}
