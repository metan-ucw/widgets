//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_TEXT_BOX_H__
#define GP_WIDGET_TEXT_BOX_H__

struct gp_widget_textbox {
	int (*on_event)(gp_widget_event *ev);
	void *event_ptr;

	/*
	 * If not NULL the textbox can contain only characters from this
	 * string, this is used as a hint when minimal textbox size is
	 * accounted for.
	 */
	const char *filter;

	int hidden:1; /* password prompt */
	int alert:1;

	size_t cur_pos;
	size_t buf_len;
	char *buf;

	char payload[];
};

enum gp_widget_text_box_flags {
	GP_WIDGET_TEXT_BOX_HIDDEN = 0x01,
};

#define GP_TEXT_BOX_FILTER_INT "0123456789"
#define GP_TEXT_BOX_FILTER_HEX "0123456789abcdefABCDEF"

struct gp_widget *gp_widget_textbox_new(const char *text, size_t str_len,
                                        const char *filter,
                                        int (*on_event)(gp_widget_event *),
                                        void *event_ptr, int flags);

int gp_widget_textbox_printf(gp_widget *self, const char *fmt, ...)
                             __attribute__((format (printf, 2, 3)));

/*
 * Clears the textbox text.
 */
void gp_widget_textbox_clear(gp_widget *self);

#endif /* GP_WIDGET_TEXT_BOX_H__ */
