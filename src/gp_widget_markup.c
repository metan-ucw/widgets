//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2020 Cyril Hrubis <metan@ucw.cz>

 */

#include <string.h>
#include <json-c/json.h>

#include <utils/gp_vec_str.h>

#include <gp_widgets.h>
#include <gp_widget_ops.h>
#include <gp_widget_render.h>

enum markup_elem_type {
	MARKUP_STR,
	MARKUP_VAR,
	MARKUP_NEWLINE,
};

enum markup_elem_attr {
	MARKUP_BOLD = 0x01,
};

struct markup_elem {
	int type:4;
	int attrs:4;
	const char *start;
	unsigned int len;
	char *var;
};

#define MARKUP_ELEMS(widget) ((void*)(widget)->markup->payload)

static int parse_markup_var(const char *markup, unsigned int attrs, struct markup_elem **elems)
{
	unsigned int i = 1;

	while (markup[i] && markup[i] != '}')
		i++;

	if (!markup[i]) {
		GP_WARN("Unfinished markup variable!");
		return -1;
	}

	i++;

	if (*elems) {
		(*elems)->type = MARKUP_VAR;
		(*elems)->start = markup;
		(*elems)->len = i;
		(*elems)->attrs = attrs;
		(*elems)++;
	}

	return i;
}

static int parse_markup_string(const char *markup, unsigned int *j, unsigned int i, unsigned int attrs, struct markup_elem **elems)
{
	if (*j == i)
		return 0;

	if (!(*elems))
		goto exit;

	(*elems)->type = MARKUP_STR;
	(*elems)->start = &markup[*j];
	(*elems)->len = i - *j;
	(*elems)->attrs = attrs;
	(*elems)++;

exit:
	*j = i;
	return 1;
}

static void markup_newline(const char *markup, unsigned int i, struct markup_elem **elems)
{
	if (!(*elems))
		return;

	(*elems)->type = MARKUP_NEWLINE;
	(*elems)->start = &markup[i];
	(*elems)->len = 1;
	(*elems)++;
}

static int parse_markup(const char *markup, struct markup_elem *elems, unsigned int *lines)
{
	unsigned int i;
	unsigned int j = 0;
	int ret = 0;
	int r;
	char prev_ch = 0;
	int attrs = 0;

	if (lines)
		*lines = 1;

	for (i = 0; markup[i]; i++) {
		switch (markup[i]) {
		case '{':
			if (prev_ch == '\\') {
				ret += parse_markup_string(markup, &j, i-1, attrs, &elems);
				j = i;
				continue;
			}

			ret += parse_markup_string(markup, &j, i, attrs, &elems);
			r = parse_markup_var(&markup[i], attrs, &elems);
			if (r < 0)
				return -1;

			i += r;
			j = i;
			ret++;
		break;
		case '\n':
			ret += parse_markup_string(markup, &j, i, attrs, &elems);
			markup_newline(markup, i, &elems);
			if (lines)
				(*lines)++;
			j = i + 1;
			ret++;
		break;
		case '*':
			if (prev_ch == '\\') {
				ret += parse_markup_string(markup, &j, i-1, attrs, &elems);
				j = i;
				continue;
			}

			ret += parse_markup_string(markup, &j, i, attrs, &elems);
			attrs ^= MARKUP_BOLD;
			j = i + 1;
		break;
		}

		prev_ch = markup[i];
	}

	ret += parse_markup_string(markup, &j, i, attrs, &elems);

	return ret;
}

static unsigned int min_w(gp_widget *self, const gp_widget_render_ctx *ctx)
{
	unsigned int max_width = 0;
	unsigned int width = 0;
	unsigned int i;
	struct markup_elem *elems = MARKUP_ELEMS(self);
	const gp_text_style *font;

	for (i = 0; i < self->markup->cnt; i++) {

		if (elems[i].attrs & MARKUP_BOLD)
			font = ctx->font_bold;
		else
			font = ctx->font;

		switch (elems[i].type) {
		case MARKUP_STR:
			width += gp_text_width_len(font, elems[i].start, elems[i].len);
		break;
		case MARKUP_VAR:
			if (elems[i].var)
				width += gp_text_width(font, elems[i].var);
			else
				width += gp_text_width_len(font, elems[i].start + 1, elems[i].len - 2);
		break;
		case MARKUP_NEWLINE:
			max_width = GP_MAX(max_width, width);
			width = 0;
		break;
		}
	}

	return GP_MAX(max_width, width);
}

static unsigned int min_h(gp_widget *self, const gp_widget_render_ctx *ctx)
{
	return ctx->padd + self->markup->lines * (ctx->padd + gp_text_ascent(ctx->font));
}

static void render(gp_widget *self, const gp_offset *offset,
                   const gp_widget_render_ctx *ctx, int flags)
{
	(void) flags;

	unsigned int x = self->x + offset->x;
	unsigned int y = self->y + offset->y;
	unsigned int w = self->w;
	unsigned int h = self->h;
	unsigned int ascent = gp_text_ascent(ctx->font);
	const gp_text_style *font;

	gp_widget_ops_blit(ctx, x, y, w, h);
	gp_fill_rect_xywh(ctx->buf, x, y, w, h, ctx->bg_color);

	unsigned int i;
	int align = GP_ALIGN_RIGHT | GP_VALIGN_BELOW;
	struct markup_elem *elems = MARKUP_ELEMS(self);

	for (i = 0; i < self->markup->cnt; i++) {

		if (elems[i].attrs & MARKUP_BOLD)
			font = ctx->font_bold;
		else
			font = ctx->font;

		switch (elems[i].type) {
		case MARKUP_STR:
			x += gp_text_ext(ctx->buf, font, x, y + ctx->padd, align,
					 ctx->text_color, ctx->bg_color,
			                 elems[i].start, elems[i].len);
		break;
		case MARKUP_VAR:
			if (!elems[i].var) {
				x += gp_text_ext(ctx->buf, font, x, y + ctx->padd, align,
				                 ctx->text_color, ctx->bg_color,
				                 elems[i].start + 1, elems[i].len - 2);
			} else {
				x += gp_text(ctx->buf, font, x, y + ctx->padd, align,
				             ctx->text_color, ctx->bg_color, elems[i].var);
			}
		break;
		case MARKUP_NEWLINE:
			y += ctx->padd + ascent;
			x = self->x + offset->x;
		break;
		}
	}
}

static void try_resize(gp_widget *self)
{
	const gp_widget_render_ctx *ctx = gp_widgets_render_ctx();

	if (self->min_w < min_w(self, ctx))
		gp_widget_resize(self);
}

void gp_widget_markup_refresh(gp_widget *self)
{
	struct markup_elem *elems = MARKUP_ELEMS(self);
	unsigned int i, var_id = 0;

	if (!self->markup->get)
		return;

	for (i = 0; i < self->markup->cnt; i++) {
		if (elems[i].type != MARKUP_VAR)
			continue;

		elems[i].var = self->markup->get(var_id++, elems[i].var);
	}

	try_resize(self);

	gp_widget_redraw(self);
}

static gp_widget *json_to_markup(json_object *json, void **uids)
{
	const char *markup = NULL;
	char *(*get)(unsigned int var_id, char *old_val) = NULL;

	(void)uids;

	json_object_object_foreach(json, key, val) {
		if (!strcmp(key, "text"))
			markup = json_object_get_string(val);
		else if (!strcmp(key, "get"))
			get = gp_widget_callback_addr(json_object_get_string(val));
		else
			GP_WARN("Invalid markup key '%s'", key);
	}

	if (!markup) {
		GP_WARN("Missing markup");
		markup = "Missing markup";
	}

	gp_widget *ret = gp_widget_markup_new(markup, get);

	return ret;
}

struct gp_widget_ops gp_widget_markup_ops = {
	.min_w = min_w,
	.min_h = min_h,
	.render = render,
	.from_json = json_to_markup,
	.id = "markup",
};

gp_widget *gp_widget_markup_new(const char *markup,
                                char *(*get)(unsigned int var_id, char *old_val))
{
	size_t payload_size = sizeof(struct gp_widget_markup);
	int elem_cnt;
	gp_widget *ret;

	elem_cnt = parse_markup(markup, NULL, NULL);
	if (elem_cnt < 0)
		return NULL;

	size_t elems_size = elem_cnt * sizeof(struct markup_elem);

	payload_size += strlen(markup) + 1;
	payload_size += elems_size;

	ret = gp_widget_new(GP_WIDGET_MARKUP, payload_size);
	if (!ret)
		return NULL;

	ret->markup->text = ret->markup->payload + elems_size;
	ret->markup->get = get;
	ret->markup->cnt = elem_cnt;

	strcpy(ret->markup->text, markup);
	parse_markup(ret->markup->text, MARKUP_ELEMS(ret), &(ret->markup->lines));

	return ret;
}

static struct markup_elem *get_var_by_id(gp_widget *self, unsigned int var_id)
{
	struct markup_elem *elems = MARKUP_ELEMS(self);
	unsigned int i, cur_id = 0;

	for (i = 0; i < self->markup->cnt; i++) {
		if (elems[i].type != MARKUP_VAR)
			continue;

		if (cur_id == var_id)
			return &elems[i];

		cur_id++;
	}

	return NULL;
}

void gp_widget_markup_set_var(gp_widget *self, unsigned int var_id, const char *fmt, ...)
{
	struct markup_elem *var;
	va_list va;

	GP_WIDGET_ASSERT(self, GP_WIDGET_MARKUP, );

	var = get_var_by_id(self, var_id);
	if (!var) {
		GP_WARN("Markup %p invalid variable id %u", self, var_id);
		return;
	}

	va_start(va, fmt);
	var->var = gp_vec_vprintf(var->var, fmt, va);
	va_end(va);

	try_resize(self);

	gp_widget_redraw(self);
}
