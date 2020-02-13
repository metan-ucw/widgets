//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <string.h>
#include <json-c/json.h>

#include <gp_widgets.h>
#include <gp_widget_ops.h>
#include <gp_widget_render.h>

static unsigned int min_w(gp_widget *self, const gp_widget_render_ctx *ctx)
{
	size_t text_len = self->tbox->buf_len - 1;
	unsigned int ret = 2 * ctx->padd;
	const char *filter = self->tbox->filter;

	if (filter)
		ret += gp_text_max_width_chars(ctx->font, filter, text_len);
	else
		ret += gp_text_max_width(ctx->font, text_len);

	return ret;
}

static unsigned int min_h(gp_widget *self, const gp_widget_render_ctx *ctx)
{
	(void)self;

	return 2 * ctx->padd + gp_text_ascent(ctx->font);
}

static const char *hidden_str(const char *buf)
{
	static const char s[] = "********************************************";
	unsigned int len = strlen(buf);

	if (len >= sizeof(s) - 1)
		return s;

	return s + sizeof(s) - len - 1;
}

static void render(gp_widget *self, const gp_offset *offset,
                   const gp_widget_render_ctx *ctx, int flags)
{
	unsigned int x = self->x + offset->x;
	unsigned int y = self->y + offset->y;
	unsigned int w = self->w;
	unsigned int h = self->h;
	const char *str;

	(void)flags;

	gp_widget_ops_blit(ctx, x, y, w, h);

	if (self->tbox->hidden)
		str = hidden_str(self->tbox->buf);
	else
		str = self->tbox->buf;

	gp_pixel color = self->selected ? ctx->sel_color : ctx->text_color;

	if (self->tbox->alert) {
		color = ctx->alert_color;
		gp_widget_render_timer(self, GP_TIMER_RESCHEDULE, 500);
	}

	gp_fill_rrect_xywh(ctx->buf, x, y, w, h, ctx->bg_color, ctx->fg_color, color);

	if (self->selected) {
		unsigned int cursor_x = x + ctx->padd;
		cursor_x += gp_text_width_len(ctx->font, str,
		                              self->tbox->cur_pos);
		gp_vline_xyh(ctx->buf, cursor_x, y + ctx->padd,
			     gp_text_ascent(ctx->font), ctx->text_color);
	}

	gp_text(ctx->buf, ctx->font,
		x + ctx->padd, y + ctx->padd,
		GP_ALIGN_RIGHT|GP_VALIGN_BELOW,
		ctx->text_color, ctx->bg_color, str);
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
	self->tbox->buf[self->tbox->cur_pos] = 0;

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

static int event(gp_widget *self, const gp_widget_render_ctx *ctx, gp_event *ev)
{
	(void)ctx;

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
		GP_WARN("At least one of size or text has to be set!");
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
                                        void *priv, int flags)
{
	gp_widget *ret;
	size_t size = sizeof(struct gp_widget_textbox) + str_len + 1;

	ret = gp_widget_new(GP_WIDGET_TEXTBOX, size);
	if (!ret)
		return NULL;

	ret->on_event = on_event;
	ret->priv = priv;
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

int gp_widget_textbox_printf(gp_widget *self, const char *fmt, ...)
{
	va_list ap;
	int len;

	GP_WIDGET_ASSERT(self, GP_WIDGET_TEXTBOX, -1);

	va_start(ap, fmt);
	len = vsnprintf(NULL, 0, fmt, ap)+1;
	va_end(ap);

	char *tmp = malloc(len);
	if (!tmp) {
		GP_DEBUG(1, "Malloc failed :(");
		return -1;
	}

	va_start(ap, fmt);
	vsprintf(tmp, fmt, ap);
	va_end(ap);

	strncpy(self->tbox->buf, tmp, self->tbox->buf_len);
	free(tmp);
	self->tbox->buf[self->tbox->buf_len - 1] = 0;

	gp_widget_redraw(self);

	return len;
}

void gp_widget_textbox_clear(gp_widget *self)
{
	GP_WIDGET_ASSERT(self, GP_WIDGET_TEXTBOX, );

	self->tbox->buf[0] = 0;
	self->tbox->cur_pos = 0;

	gp_widget_redraw(self);
}
