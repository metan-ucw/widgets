//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2020 Cyril Hrubis <metan@ucw.cz>

 */

#include <string.h>
#include <core/gp_debug.h>
#include <core/gp_common.h>
#include <gp_widget.h>
#include <gp_widget_event.h>
#include <gp_widget_ops.h>
#include <gp_widget_render.h>

extern struct gp_widget_ops gp_widget_grid_ops;
extern struct gp_widget_ops gp_widget_tabs_ops;
extern struct gp_widget_ops gp_widget_button_ops;
extern struct gp_widget_ops gp_widget_label_ops;
extern struct gp_widget_ops gp_widget_check_box_ops;
extern struct gp_widget_ops gp_widget_progress_bar_ops;
extern struct gp_widget_ops gp_widget_spinner_ops;
extern struct gp_widget_ops gp_widget_slider_ops;
extern struct gp_widget_ops gp_widget_textbox_ops;
extern struct gp_widget_ops gp_widget_radio_button_ops;
extern struct gp_widget_ops gp_widget_table_ops;
extern struct gp_widget_ops gp_widget_pixmap_ops;
extern struct gp_widget_ops gp_widget_scroll_area_ops;

static struct gp_widget_ops *widget_ops[] = {
	[GP_WIDGET_GRID]        = &gp_widget_grid_ops,
	[GP_WIDGET_TABS]        = &gp_widget_tabs_ops,
	[GP_WIDGET_BUTTON]      = &gp_widget_button_ops,
	[GP_WIDGET_CHECKBOX]    = &gp_widget_check_box_ops,
	[GP_WIDGET_LABEL]       = &gp_widget_label_ops,
	[GP_WIDGET_SPINNER]     = &gp_widget_spinner_ops,
	[GP_WIDGET_SLIDER]      = &gp_widget_slider_ops,
	[GP_WIDGET_PROGRESSBAR] = &gp_widget_progress_bar_ops,
	[GP_WIDGET_TEXTBOX]     = &gp_widget_textbox_ops,
	[GP_WIDGET_RADIOBUTTON] = &gp_widget_radio_button_ops,
	[GP_WIDGET_TABLE]       = &gp_widget_table_ops,
	[GP_WIDGET_PIXMAP]      = &gp_widget_pixmap_ops,
	[GP_WIDGET_SCROLL_AREA] = &gp_widget_scroll_area_ops,
};

const struct gp_widget_ops *gp_widget_ops(gp_widget *self)
{
	if (self->type > GP_ARRAY_SIZE(widget_ops)) {
		GP_WARN("Invalid widget type %u", self->type);
		return NULL;
	}

	return widget_ops[self->type];
}

const char *gp_widget_type_id(gp_widget *self)

{
	if (!self)
		return "NULL";

	return gp_widget_ops(self)->id;
}

const struct gp_widget_ops *gp_widget_ops_by_id(const char *id)
{
	unsigned int i;

	for (i = 0; i < GP_ARRAY_SIZE(widget_ops); i++) {
		if (!strcmp(id, widget_ops[i]->id))
			return widget_ops[i];
	}

	return NULL;
}

const char *gp_widget_type_name(enum gp_widget_type type)
{
	if (type > GP_ARRAY_SIZE(widget_ops))
		return "Unknown";

	return widget_ops[type]->id;
}

void gp_widget_free(gp_widget *self)
{
	const struct gp_widget_ops *ops;

	if (!self)
		return;

	ops = gp_widget_ops(self);
	if (!ops->free)
		free(self);
	else
		ops->free(self);
}

unsigned int gp_widget_min_w(gp_widget *self, const gp_widget_render_ctx *ctx)
{
	const struct gp_widget_ops *ops;

	if (!self)
		return 0;

	if (self->no_resize)
		return self->min_w;

	ops = gp_widget_ops(self);
	if (!ops->min_w) {
		GP_WARN("%s->min_w() not implemented!", ops->id);
		return 0;
	}

	self->min_w = ops->min_w(self, ctx);
	return self->min_w;
}

unsigned int gp_widget_min_h(gp_widget *self, const gp_widget_render_ctx *ctx)
{
	const struct gp_widget_ops *ops;

	if (!self)
		return 0;

	if (self->no_resize)
		return self->min_h;

	ops = gp_widget_ops(self);
	if (!ops->min_h) {
		GP_WARN("%s->min_h() not implemented!", ops->id);
		return 0;
	}

	self->min_h = ops->min_h(self, ctx);
	return self->min_h;
}

unsigned int gp_widget_align(gp_widget *self)
{
	if (!self)
		return 0;

	return self->align;
}

static const char *halign_to_str(int align)
{
	switch (GP_HALIGN_MASK & align) {
	case GP_HCENTER_WEAK:
		return "HCENTER_WEAK";
	case GP_HCENTER:
		return "HCENTER";
	case GP_LEFT:
		return "LEFT";
	case GP_RIGHT:
		return "RIGHT";
	default:
		return "HFILL";
	}
}

static const char *valign_to_str(int align)
{
	switch (GP_VALIGN_MASK & align) {
	case GP_VCENTER_WEAK:
		return "VCENTER_WEAK";
	case GP_VCENTER:
		return "VCENTER";
	case GP_TOP:
		return "TOP";
	case GP_BOTTOM:
		return "BOTTOM";
	default:
		return "VFILL";
	}
}

static void widget_resize(gp_widget *self,
                          unsigned int w, unsigned int h)
{
	unsigned int dw = w - self->min_w;
	unsigned int dh = h - self->min_h;

	self->redraw = 1;

	switch (GP_HALIGN_MASK & self->align) {
	case GP_HCENTER_WEAK:
	case GP_HCENTER:
		self->x = (dw+1)/2;
	break;
	case GP_RIGHT:
		self->x = dw;
	break;
	default:
		self->x = 0;
	break;
	}

	if (self->align & GP_HFILL)
		self->w = w;
	else
		self->w = self->min_w;

	switch (GP_VALIGN_MASK & self->align) {
	case GP_VCENTER_WEAK:
	case GP_VCENTER:
		self->y = (dh+1)/2;
	break;
	case GP_BOTTOM:
		self->y = dh;
	break;
	default:
		self->y = 0;
	break;
	}

	if (self->align & GP_VFILL)
		self->h = h;
	else
		self->h = self->min_h;

	GP_DEBUG(4,
	         "Placing widget %p (%s) min size %ux%u %s|%s to %ux%u = %ux%u-%ux%u",
		 self, gp_widget_type_id(self),
	         self->min_w, self->min_h,
		 halign_to_str(self->align), valign_to_str(self->align),
		 w, h, self->x, self->y, self->w, self->h);
}

void gp_widget_ops_distribute_size(gp_widget *self, const gp_widget_render_ctx *ctx,
                                   unsigned int w, unsigned int h,
                                   int new_wh)
{
	const struct gp_widget_ops *ops = gp_widget_ops(self);

	if (self->no_resize && !new_wh)
		return;

	self->no_resize = 1;

	if (self->min_w > w) {
		GP_WARN("%p (%s) min_w=%u > w=%u",
			self, gp_widget_type_id(self),
			self->min_w, w);
		w = self->min_w;
	}

	if (self->min_h > h) {
		GP_WARN("%p (%s) min_h=%u > h=%u",
			self, gp_widget_type_id(self),
			self->min_h, h);
		h = self->min_h;
	}

	unsigned int old_w = self->w;
	unsigned int old_h = self->h;

	widget_resize(self, w, h);

	if (self->w != old_w || self->h != old_h)
		gp_widget_send_event(self, GP_WIDGET_EVENT_RESIZE, ctx);

	if (ops->distribute_size)
		ops->distribute_size(self, ctx, 1);
}

void gp_widget_calc_size(gp_widget *self, const gp_widget_render_ctx *ctx,
                         unsigned int w, unsigned int h, int new_wh)
{
	if (!self)
		return;

	if (self->no_resize && !new_wh)
		return;

	GP_DEBUG(1, "Recalculating layout %p", self);

	gp_widget_min_w(self, ctx);
	gp_widget_min_h(self, ctx);

	w = GP_MAX(self->min_w, w);
	h = GP_MAX(self->min_h, h);

	/* Avoid crashes on empty layout */
	w = GP_MAX(w, 1u);
	h = GP_MAX(h, 1u);

	gp_widget_ops_distribute_size(self, ctx, w, h, new_wh);
}

void gp_widget_ops_render(gp_widget *self, const gp_offset *offset,
                          const gp_widget_render_ctx *ctx, int flags)
{
	const struct gp_widget_ops *ops;

	if (!self->redraw_child && !gp_widget_should_redraw(self, flags))
		return;

	ops = gp_widget_ops(self);
	if (!ops->render) {
		GP_WARN("%s->render not implemented!", ops->id);
		return;
	}

	gp_coord x = (gp_coord)self->x + offset->x;
	gp_coord y = (gp_coord)self->y + offset->y;

	if (ctx->bbox) {
		gp_bbox bbox = gp_bbox_pack(x, y, self->w, self->h);

		if (!gp_bbox_intersects(*ctx->bbox, bbox)) {
			GP_DEBUG(3, "Widget %p %s %ix%i %ux%u-%ux%u out of " GP_BBOX_FMT,
			         self, ops->id, x, y, self->x, self->y, self->w, self->h,
				 GP_BBOX_PARS(*ctx->bbox));
			return;
		}
	}

	GP_DEBUG(3, "rendering widget %p %s (%u) %ux%u %ux%u-%ux%u flags=%x",
	         self, ops->id, self->type, x, y, self->x, self->y,
	         self->w, self->h, flags);

	ops->render(self, offset, ctx, flags);

	if (ctx->flip)
		GP_DEBUG(3, "render bbox " GP_BBOX_FMT, GP_BBOX_PARS(*ctx->flip));

	self->redraw = 0;
	self->redraw_child = 0;

	//gp_rect_xywh(render->buf, x, y, self->w, self->h, 0x00ff00);
}

static void select_widget(gp_widget *self, int sel)
{
	if (!self)
		return;

	if (self->selected == sel)
		return;

	self->selected = sel;

	gp_widget_redraw(self);
}

static const char *select_to_str(int flag)
{
	switch (flag) {
	case GP_SELECT_OUT:
		return "select_out";
	case GP_SELECT_IN:
		return "select_in";
	case GP_SELECT_NEXT:
		return "select_next";
	case GP_SELECT_PREV:
		return "select_prev";
	case GP_SELECT_LEFT:
		return "select_left";
	case GP_SELECT_RIGHT:
		return "select_right";
	case GP_SELECT_UP:
		return "select_up";
	case GP_SELECT_DOWN:
		return "select_down";
	}

	return "???";
}

int gp_widget_ops_render_select_xy(gp_widget *self, const gp_widget_render_ctx *ctx,
                                   unsigned int x, unsigned int y)
{
	const struct gp_widget_ops *ops;

	GP_DEBUG(3, "select event %p (%s) %ux%u",
		 self, gp_widget_type_id(self), x, y);

	if (!self)
		return 0;

	ops = gp_widget_ops(self);
	if (!ops->event)
		return 0;

	if (ops->select_xy)
		return ops->select_xy(self, ctx, x, y);

	if (self->selected)
		return 0;

	select_widget(self, 1);
	return 1;
}

int gp_widget_ops_render_select(gp_widget *self, int flag)
{
	const struct gp_widget_ops *ops;

	GP_DEBUG(3, "select event %p (%s) %s",
		 self, gp_widget_type_id(self), select_to_str(flag));

	if (!self)
		return 0;

	ops = gp_widget_ops(self);

	if (!ops->event)
		return 0;

	if (ops->select)
		return ops->select(self, flag);

	if (flag) {
		if (self->selected)
			return 0;

		select_widget(self, 1);
		return 1;
	}

	select_widget(self, 0);
	return 1;
}

static int handle_select(gp_widget *self, const gp_widget_render_ctx *ctx, gp_event *ev)
{
	switch (ev->type) {
	case GP_EV_KEY:
		if (ev->code == GP_EV_KEY_UP)
			return 0;

		switch (ev->val.val) {
		case GP_KEY_TAB:
			if (gp_event_get_key(ev, GP_KEY_LEFT_SHIFT) ||
			    gp_event_get_key(ev, GP_KEY_RIGHT_SHIFT))
				gp_widget_ops_render_select(self, GP_SELECT_PREV);
			else
				gp_widget_ops_render_select(self, GP_SELECT_NEXT);

			return 1;
		case GP_BTN_LEFT:
			gp_widget_ops_render_select_xy(self, ctx, ev->cursor_x, ev->cursor_y);
			return 0;
		}

		if (!gp_event_get_key(ev, GP_KEY_LEFT_SHIFT) &&
		    !gp_event_get_key(ev, GP_KEY_RIGHT_SHIFT))
			return 0;

		switch (ev->val.val) {
		case GP_KEY_LEFT:
			gp_widget_ops_render_select(self, GP_SELECT_LEFT);
			return 1;
		case GP_KEY_RIGHT:
			gp_widget_ops_render_select(self, GP_SELECT_RIGHT);
			return 1;
		case GP_KEY_UP:
			gp_widget_ops_render_select(self, GP_SELECT_UP);
			return 1;
		case GP_KEY_DOWN:
			gp_widget_ops_render_select(self, GP_SELECT_DOWN);
			return 1;
		}
	break;
	}

	return 0;
}

int gp_widget_input_event(gp_widget *self, const gp_widget_render_ctx *ctx,
                          gp_event *ev)
{
	if (handle_select(self, ctx, ev))
		return 1;

	return gp_widget_ops_event(self, ctx, ev);
}

int gp_widget_ops_event(gp_widget *self, const gp_widget_render_ctx *ctx, gp_event *ev)
{
	const struct gp_widget_ops *ops;
	int handled;

	if (!self)
		return 0;

	ops = gp_widget_ops(self);
	if (!ops->event)
		return 0;

	GP_DEBUG(3, "event widget %p (%s)", self, ops->id);

	handled = ops->event(self, ctx, ev);

	if (!handled)
		handled = gp_widget_send_event(self, GP_WIDGET_EVENT_INPUT, ctx, ev);

	return handled;
}

void gp_widget_render(gp_widget *self, const gp_widget_render_ctx *ctx, int new_wh)
{
	GP_DEBUG(1, "rendering layout %p", self);
	gp_widget_calc_size(self, ctx,
	                    gp_pixmap_w(ctx->buf),
	                    gp_pixmap_h(ctx->buf), new_wh);

	gp_offset offset = {
		.x = 0,
		.y = 0,
	};

	gp_widget_ops_render(self, &offset, ctx, 0);
}

void gp_widget_redraw_child(gp_widget *self)
{
	if (!self)
		return;

	if (self->redraw_child)
		return;

	GP_DEBUG(3, "Widget %p (%s) redraw_child=1", self, gp_widget_type_id(self));

	self->redraw_child = 1;

	gp_widget_redraw_child(self->parent);
}

void gp_widget_redraw(gp_widget *self)
{
	if (!self)
		return;

	if (self->redraw)
		return;

	GP_DEBUG(3, "Widget %p (%s) redraw=1", self, gp_widget_type_id(self));

	self->redraw = 1;

	gp_widget_redraw_child(self->parent);
}

void gp_widget_resize(gp_widget *self)
{
	if (!self)
		return;

	if (!self->no_resize)
		return;

	GP_DEBUG(3, "Widget %p (%s) no_resize=0", self, gp_widget_type_id(self));

	self->no_resize = 0;

	gp_widget_resize(self->parent);
}
