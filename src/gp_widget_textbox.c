//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <string.h>
#include <json-c/json.h>

#include <gp_widgets.h>
#include <gp_widget_ops.h>
#include <gp_widget_render.h>

static unsigned int min_w(gp_widget *self)
{
	size_t text_len = self->tbox->buf_len - 1;
	unsigned int ret = 2 * cfg->padd;
	const char *filter = self->tbox->filter;

	if (filter)
		ret += gp_text_max_width_chars(cfg->font, filter, text_len);
	else
		ret += gp_text_max_width(cfg->font, text_len);

	return ret;
}

static unsigned int min_h(gp_widget *self)
{
	(void)self;

	return 2 * cfg->padd + gp_text_ascent(cfg->font);
}

static const char *hidden_str(const char *buf)
{
	static const char s[] = "********************************************";
	unsigned int len = strlen(buf);

	if (len >= sizeof(s) - 1)
		return s;

	return s + sizeof(s) - len - 1;
}

static void render(gp_widget *self,
                   struct gp_widget_render *render, int flags)
{
	unsigned int x = self->x;
	unsigned int y = self->y;
	unsigned int w = self->w;
	unsigned int h = self->h;
	const char *str;

	(void)flags;

	if (self->tbox->hidden)
		str = hidden_str(self->tbox->buf);
	else
		str = self->tbox->buf;

	gp_pixel color = self->selected ? cfg->sel_color : cfg->text_color;

	if (self->tbox->alert) {
		color = cfg->alert_color;
		gp_widget_render_timer(self, GP_TIMER_RESCHEDULE, 500);
	}

	gp_fill_rrect_xywh(render->buf, x, y, w, h, cfg->bg_color, cfg->fg_color, color);

	if (self->selected) {
		unsigned int cursor_x = x + cfg->padd;
		cursor_x += gp_text_width_len(cfg->font, str,
		                              self->tbox->cur_pos);
		gp_vline_xyh(render->buf, cursor_x, y + cfg->padd,
			     gp_text_ascent(cfg->font), cfg->text_color);
	}

	gp_text(render->buf, cfg->font,
		x + cfg->padd, y + cfg->padd,
		GP_ALIGN_RIGHT|GP_VALIGN_BELOW,
		cfg->text_color, cfg->bg_color, str);
}


static void schedule_alert(gp_widget *self)
{
	self->tbox->alert = 1;
	gp_widget_redraw(self);
}

static int filter(const char *filter, char ch)
{
	if (!filter)
		return 0;

	while (*filter) {
		if (*filter++ == ch)
			return 0;
	}

	return 1;
}

static void ascii_key(gp_widget *self, char ch)
{
	unsigned int i;

	if (self->tbox->cur_pos + 1 >= self->tbox->buf_len) {
		schedule_alert(self);
		return;
	}

	int ret = gp_widget_send_event(self, GP_WIDGET_EVENT_FILTER, (long)ch);

	if (ret || filter(self->tbox->filter, ch)) {
		schedule_alert(self);
		return;
	}

	if (self->tbox->buf[self->tbox->cur_pos]) {
		if (self->tbox->buf[self->tbox->buf_len-2]) {
			schedule_alert(self);
			return;
		}

		for (i = self->tbox->buf_len-1; i > self->tbox->cur_pos; i--)
			self->tbox->buf[i] = self->tbox->buf[i-1];
	}

	self->tbox->buf[self->tbox->cur_pos++] = ch;

	gp_widget_send_event(self, GP_WIDGET_EVENT_EDIT);

	gp_widget_redraw(self);
}

static void key_backspace(gp_widget *self)
{
	unsigned int i;

	if (self->tbox->cur_pos <= 0) {
		schedule_alert(self);
		return;
	}

	self->tbox->cur_pos--;

	for (i = self->tbox->cur_pos; i < self->tbox->buf_len-1; i++)
		self->tbox->buf[i] = self->tbox->buf[i + 1];

	gp_widget_send_event(self, GP_WIDGET_EVENT_EDIT);

	gp_widget_redraw(self);
}

static void key_left(gp_widget *self)
{
	if (self->tbox->cur_pos > 0) {
		self->tbox->cur_pos--;
		gp_widget_redraw(self);
	}
}

static void key_right(gp_widget *self)
{
	if (self->tbox->cur_pos < self->tbox->buf_len &&
	    self->tbox->buf[self->tbox->cur_pos]) {
		self->tbox->cur_pos++;
		gp_widget_redraw(self);
	}
}

static void key_home(gp_widget *self)
{
	if (self->tbox->cur_pos == 0)
		return;

	self->tbox->cur_pos = 0;
	gp_widget_redraw(self);
}

static void key_end(gp_widget *self)
{
	if (!self->tbox->buf[self->tbox->cur_pos])
		return;

	self->tbox->cur_pos = strlen(self->tbox->buf);
	gp_widget_redraw(self);
}

static int event(gp_widget *self, gp_event *ev)
{
	switch (ev->type) {
	//TODO: Mouse clicks
	case GP_EV_KEY:
		if (ev->code == GP_EV_KEY_UP)
			return 0;

		switch (ev->val.val) {
		case GP_KEY_TAB:
			return 0;
		case GP_KEY_ENTER:
			if (ev->code == GP_EV_KEY_DOWN)
				gp_widget_send_event(self, GP_WIDGET_EVENT_ACTION);
			return 1;
		case GP_KEY_LEFT:
			key_left(self);
			return 1;
		case GP_KEY_RIGHT:
			key_right(self);
			return 1;
		case GP_KEY_HOME:
			key_home(self);
			return 1;
		case GP_KEY_END:
			key_end(self);
			return 1;
		case GP_KEY_BACKSPACE:
			key_backspace(self);
			return 1;
		}

		if (ev->val.key.ascii) {
			ascii_key(self, ev->val.key.ascii);
			return 1;
		}
	break;
	case GP_EV_TMR:
		self->tbox->alert = 0;
		gp_widget_redraw(self);
		return 1;
	break;
	}

	return 0;
}

static gp_widget *json_to_textbox(json_object *json, void **uid)
{
	const char *text = NULL;
	int flags = 0;
	int len = 0;

	(void)uid;

	json_object_object_foreach(json, key, val) {
		if (!strcmp(key, "text"))
			text = json_object_get_string(val);
		else if (!strcmp(key, "size"))
			len = json_object_get_int(val);
		else if (!strcmp(key, "hidden"))
			flags |= json_object_get_boolean(val) ? GP_WIDGET_TEXT_BOX_HIDDEN : 0;
		else
			GP_WARN("Invalid textbox key '%s'", key);
	}

	if (len <= 0 && !text) {
		GP_WARN("At least one of len or text has to be set!");
		return NULL;
	}

	if (len <= 0)
		len = strlen(text);

	return gp_widget_textbox_new(text, len, NULL, NULL, NULL, flags);
}

struct gp_widget_ops gp_widget_textbox_ops = {
	.min_w = min_w,
	.min_h = min_h,
	.render = render,
	.event = event,
	.from_json = json_to_textbox,
	.id = "textbox",
};

struct gp_widget *gp_widget_textbox_new(const char *text, size_t str_len,
                                        const char *filter,
                                        int (*on_event)(gp_widget_event *),
                                        void *event_ptr, int flags)
{
	gp_widget *ret;
	size_t size = sizeof(struct gp_widget_textbox) + str_len + 1;

	ret = gp_widget_new(GP_WIDGET_TEXTBOX, size);
	if (!ret)
		return NULL;

	ret->on_event = on_event;
	ret->on_event_ptr = event_ptr;
	ret->tbox->buf_len = str_len + 1;
	ret->tbox->buf = ret->tbox->payload;
	ret->tbox->filter = filter;

	if (flags & GP_WIDGET_TEXT_BOX_HIDDEN)
		ret->tbox->hidden = 1;

	if (text) {
		ret->tbox->cur_pos = strlen(text);
		strncpy(ret->tbox->buf, text, str_len);
		ret->tbox->buf[str_len] = 0;
	}

	return ret;
}
