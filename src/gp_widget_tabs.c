//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <string.h>
#include <json-c/json.h>

#include <core/gp_debug.h>
#include <core/gp_common.h>

#include <gp_widgets.h>
#include <gp_widget_ops.h>
#include <gp_string.h>

static void init(gp_widget *self)
{
	unsigned int i;

	for (i = 0; i < self->tabs->count; i++)
		gp_widget_init(self->tabs->widgets[i], self);
}

static gp_size tab_w(gp_widget *self, const gp_widget_render_cfg *cfg,
                     unsigned int tab)
{
	const char *label = self->tabs->labels[tab];

	return gp_text_width(cfg->font_bold, label) + 2 * cfg->padd;
}

static unsigned int min_w(gp_widget *self, const gp_widget_render_cfg *cfg)
{
	unsigned int i, max_min_w = 0, tabs_width = 0;

	for (i = 0; i < self->tabs->count; i++) {
		unsigned int min_w = gp_widget_min_w(self->tabs->widgets[i], cfg);
		max_min_w = GP_MAX(max_min_w, min_w);

		tabs_width += tab_w(self, cfg, i);
	}

	return GP_MAX(max_min_w + 2 * cfg->padd, tabs_width);
}

static gp_size title_h(gp_widget *self, const gp_widget_render_cfg *cfg)
{
	(void) self;

	return gp_text_ascent(cfg->font) + 2 * cfg->padd;
}

static unsigned int min_h(gp_widget *self, const gp_widget_render_cfg *cfg)
{
	unsigned int i, max_min_h = 0;

	for (i = 0; i < self->tabs->count; i++) {
		unsigned int min_h = gp_widget_min_h(self->tabs->widgets[i], cfg);

		max_min_h = GP_MAX(max_min_h, min_h);
	}

	return max_min_h + title_h(self, cfg) + 2 * cfg->padd;
}

static gp_size payload_x(gp_widget *self, const gp_widget_render_cfg *cfg)
{
	return self->x + cfg->padd;
}

static gp_size payload_y(gp_widget *self, const gp_widget_render_cfg *cfg)
{
	return self->y + title_h(self, cfg) + cfg->padd;
}

static gp_size payload_w(gp_widget *self, const gp_widget_render_cfg *cfg)
{
	return self->w - 2 * cfg->padd;
}

static gp_size payload_h(gp_widget *self, const gp_widget_render_cfg *cfg)
{
	return self->h - title_h(self, cfg) - 2 * cfg->padd;
}

static void distribute_size(gp_widget *self, const gp_widget_render_cfg *cfg,
                            int new_wh)
{
	unsigned int i;
	unsigned int x = payload_x(self, cfg);
	unsigned int y = payload_y(self, cfg);
	unsigned int w = payload_w(self, cfg);
	unsigned int h = payload_h(self, cfg);

	for (i = 0; i < self->tabs->count; i++) {

		if (!self->tabs->widgets[i])
			continue;

		gp_widget_ops_distribute_size(self->tabs->widgets[i], cfg,
		                              x, y, w, h, new_wh);
	}
}

static int active_first(gp_widget *self)
{
	return self->tabs->active_tab == 0;
}

static void render(gp_widget *self, const gp_widget_render_cfg *cfg, int flags)
{
	gp_widget *widget = self->tabs->widgets[self->tabs->active_tab];

	unsigned int i;
	unsigned int x = self->x;
	unsigned int tab_h = gp_text_ascent(cfg->font) + 2 * cfg->padd;
	unsigned int act_x, act_w;

	if (!widget) {
		gp_fill_rect_xywh(cfg->buf, self->x, self->y,
				  self->w, self->h, cfg->bg_color);
	} else {
		gp_fill_rect_xywh(cfg->buf, self->x, self->y,
		                  self->w, widget->y - self->y, cfg->bg_color);
	}

	for (i = 0; i < self->tabs->count; i++) {
		const char *label = self->tabs->labels[i];
		int is_active = self->tabs->active_tab == i;
		gp_text_style *font = is_active ? cfg->font_bold : cfg->font;

		unsigned int w = gp_text_width(cfg->font_bold, label) + 2 * cfg->padd;

		if (is_active) {
			act_x = x;
			act_w = w;
		}

		if (is_active && self->tabs->title_selected) {
			gp_hline_xyw(cfg->buf,
				    x + cfg->padd/2,
				    self->y + tab_h - cfg->padd,
				    w - cfg->padd, cfg->sel_color);
		}

		gp_text(cfg->buf, font, x + w/2, self->y + cfg->padd,
			GP_ALIGN_CENTER|GP_VALIGN_BELOW,
			cfg->text_color, cfg->bg_color, label);

		x += w;

		if (x < self->x + self->w)
			gp_vline_xyh(cfg->buf, x-1, self->y+1, tab_h-1, cfg->text_color);
	}

	if (!active_first(self))
		gp_hline_xxy(cfg->buf, self->x, act_x-1, self->y + tab_h, cfg->text_color);

	gp_hline_xxy(cfg->buf, act_x + act_w - 1, self->x + self->w-1,
		     self->y + tab_h, cfg->text_color);

	gp_rrect_xywh(cfg->buf, self->x, self->y, self->w, self->h, cfg->text_color);

	if (!widget)
		return;

	gp_fill_rect_xyxy(cfg->buf, self->x+1, widget->y + widget->h,
	                  self->x + self->w-2, self->y + self->h-2, cfg->bg_color);
	gp_fill_rect_xyxy(cfg->buf, widget->x + widget->w, widget->y,
	                  self->x + self->w-2, widget->y + widget->h, cfg->bg_color);
	gp_fill_rect_xywh(cfg->buf, self->x+1, widget->y,
	                  widget->x - self->x-1, widget->h, cfg->bg_color);

	if (self->redraw_subtree) {
		self->redraw_subtree = 0;
		flags |= 1;
	}

	gp_widget_ops_render(widget, cfg, flags);
}

static void set_tab(gp_widget *self, unsigned int tab)
{
	if (tab == self->tabs->active_tab)
		return;

	self->tabs->active_tab = tab;
	self->redraw_subtree = 1;
	gp_widget_redraw(self);
}

static gp_widget *active_tab_widget(gp_widget *self)
{
	return self->tabs->widgets[self->tabs->active_tab];
}

static void tab_left(gp_widget *self)
{
	unsigned int tab;

	if (self->tabs->active_tab > 0)
		tab = self->tabs->active_tab - 1;
	else
		tab = self->tabs->count - 1;

	set_tab(self, tab);
}

static void tab_right(gp_widget *self)
{
	unsigned int tab;

	if (self->tabs->active_tab + 1 < self->tabs->count)
		tab = self->tabs->active_tab + 1;
	else
		tab = 0;

	set_tab(self, tab);
}

static int event(gp_widget *self, const gp_widget_render_cfg *cfg, gp_event *ev)
{
	if (self->tabs->widget_selected)
		return gp_widget_ops_event(active_tab_widget(self), cfg, ev);

	if (!self->tabs->title_selected)
		return 0;

	switch (ev->type) {
	case GP_EV_KEY:
		if (ev->code == GP_EV_KEY_UP)
			return 0;

		switch (ev->val.val) {
		case GP_KEY_LEFT:
			tab_left(self);
			return 1;
		break;
		case GP_KEY_RIGHT:
			tab_right(self);
			return 1;
		break;
		}
	break;
	}

	return 0;
}

static int select_out(gp_widget *self)
{
	if (self->tabs->widget_selected)
		return gp_widget_ops_render_select(active_tab_widget(self), GP_SELECT_OUT);

	if (self->tabs->title_selected) {
		self->tabs->title_selected = 0;
		gp_widget_redraw(self);
	}

	return 0;
}

static int select_prev(gp_widget *self)
{
	gp_widget *w = active_tab_widget(self);

	if (self->tabs->title_selected)
		return 0;

	if (self->tabs->widget_selected) {
		gp_widget_ops_render_select(w, GP_SELECT_OUT);
		self->tabs->widget_selected = 0;
		self->tabs->title_selected = 1;
		gp_widget_redraw(self);
		return 1;
	}

	if (gp_widget_ops_render_select(w, GP_SELECT_IN))
		return 1;

	self->tabs->title_selected = 1;
	gp_widget_redraw(self);
	return 1;
}

static int select_next(gp_widget *self)
{
	gp_widget *w = active_tab_widget(self);

	if (self->tabs->title_selected) {
		if (!gp_widget_ops_render_select(w, GP_SELECT_IN))
			return 0;
		self->tabs->title_selected = 0;
		self->tabs->widget_selected = 1;
		return 1;
	}

	if (self->tabs->widget_selected)
		return 0;

	self->tabs->title_selected = 1;
	gp_widget_redraw(self);
	return 1;
}

static int select_event(gp_widget *self, int sel)
{
	gp_widget *w = active_tab_widget(self);

	if (self->tabs->widget_selected) {
		if (gp_widget_ops_render_select(w, sel))
			return 1;
	}

	switch (sel) {
	case GP_SELECT_OUT:
		return select_out(self);
	case GP_SELECT_LEFT:
		return 0;
	case GP_SELECT_RIGHT:
		return 0;
	case GP_SELECT_UP:
	case GP_SELECT_PREV:
		return select_prev(self);
	case GP_SELECT_DOWN:
	case GP_SELECT_NEXT:
		return select_next(self);
	}

	return 0;
}

static void select_tab(gp_widget *self, const gp_widget_render_cfg *cfg,
                       unsigned int x)
{
	unsigned int i, cx = self->x;

	for (i = 0; i < self->tabs->count; i++) {
		unsigned int w = tab_w(self, cfg, i);

		if (x <= cx + w)
			break;

		cx += w;
	}

	if (i == self->tabs->count)
		return;

	set_tab(self, i);
}


static int select_title(gp_widget *self, const gp_widget_render_cfg *cfg,
                        unsigned int x)
{
	self->tabs->title_selected = 1;

	if (self->tabs->widget_selected) {
		gp_widget_ops_render_select(active_tab_widget(self), GP_SELECT_OUT);
		self->tabs->widget_selected = 0;
	}

	select_tab(self, cfg, x);

	return 1;
}

static int select_widget(gp_widget *self, const gp_widget_render_cfg *cfg,
                         unsigned int x, unsigned int y)
{
	if (!gp_widget_ops_render_select_xy(active_tab_widget(self), cfg, x, y))
		return 0;

	if (self->tabs->title_selected) {
		self->tabs->title_selected = 0;
		gp_widget_redraw(self);
	}

	self->tabs->widget_selected = 1;
	return 1;
}

static int select_xy(gp_widget *self, const gp_widget_render_cfg *cfg,
                     unsigned int x, unsigned int y)
{
	if (y > self->y + title_h(self, cfg))
		return select_widget(self, cfg, x, y);

	return select_title(self, cfg, x);
}

static gp_widget *json_to_tabs(json_object *json, void **uids)
{
	json_object *widgets = NULL;
	json_object *labels = NULL;

	json_object_object_foreach(json, key, val) {
		if (!strcmp(key, "labels"))
			labels = val;
		else if (!strcmp(key, "widgets"))
			widgets = val;
		else
			GP_WARN("Invalid tabs key '%s'", key);
	}

	if (!labels) {
		GP_WARN("Missing tabs array!");
		return NULL;
	}

	if (!widgets) {
		GP_WARN("Missing widgets array!");
		return NULL;
	}

	if (!json_object_is_type(labels, json_type_array)) {
		GP_WARN("Tabs has to be array of strings!");
		return NULL;
	}

	if (!json_object_is_type(widgets, json_type_array)) {
		GP_WARN("Tabs has to be array of strings!");
		return NULL;
	}

	unsigned int i, tab_count = json_object_array_length(labels);
	const char *tab_labels[tab_count];

	for (i = 0; i < tab_count; i++) {
		json_object *label = json_object_array_get_idx(labels, i);
		tab_labels[i] = json_object_get_string(label);

		if (!tab_labels[i])
			GP_WARN("Tab title %i must be string!", i);
	}

	gp_widget *ret = gp_widget_tabs_new(tab_count, 0, tab_labels);

	for (i = 0; i < tab_count; i++) {
		json_object *json_widget = json_object_array_get_idx(widgets, i);

		if (!json_widget) {
			GP_WARN("Not enough widgets to fill tabs!");
			return ret;
		}

		ret->tabs->widgets[i] = gp_widget_from_json(json_widget, uids);

		if (ret->tabs->widgets[i])
			ret->tabs->widgets[i]->parent = ret;
	}


	return ret;
}

static void free_(gp_widget *self)
{
	unsigned int i;

	for (i = 0; i < self->tabs->count; i++)
		gp_widget_free(self->tabs->widgets[i]);
}

struct gp_widget_ops gp_widget_tabs_ops = {
	.init = init,
	.min_w = min_w,
	.min_h = min_h,
	.render = render,
	.event = event,
	.free = free_,
	.select = select_event,
	.select_xy = select_xy,
	.distribute_size = distribute_size,
	.from_json = json_to_tabs,
	.id = "tabs",
};

gp_widget *gp_widget_tabs_new(unsigned int tabs, unsigned int active_tab,
                              const char *tab_labels[])
{
	size_t size = sizeof(struct gp_widget_tabs) + tabs * sizeof(void*);

	size += gp_string_arr_size(tab_labels, tabs);

	gp_widget *ret = gp_widget_new(GP_WIDGET_TABS, size);
	if (!ret)
		return NULL;

	memset(ret->tabs, 0, size);

	if (active_tab >= tabs) {
		GP_WARN("Active tab %u >= tabs %u", active_tab, tabs);
		active_tab = 0;
	}

	void *payload = ret->tabs->payload;

	ret->tabs->count = tabs;
	ret->tabs->active_tab = active_tab;
	ret->tabs->widgets = payload;
	payload += tabs * sizeof(void*);
	ret->tabs->labels = gp_string_arr_copy(tab_labels, tabs, payload);

	return ret;
}

gp_widget *gp_widget_tabs_put(gp_widget *self, unsigned int tab,
                              gp_widget *widget)
{
	gp_widget *ret;

	GP_WIDGET_ASSERT(self, GP_WIDGET_TABS, NULL);

	if (tab >= self->tabs->count) {
		GP_WARN("Invalid tabs index %u", tab);
		return NULL;
	}

	if (widget && widget->parent) {
		//TODO: remove from parent!!!
		return NULL;
	}

	ret = self->tabs->widgets[tab];
	if (ret)
		ret->parent = NULL;

	self->tabs->widgets[tab] = widget;
	if (widget)
		widget->parent = self;

	gp_widget_resize(self);
	//TODO: Redraw as well?

	return ret;
}
