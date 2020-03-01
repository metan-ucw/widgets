//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_LABEL_H__
#define GP_WIDGET_LABEL_H__

#include <stdlib.h>

struct gp_widget_label {
	unsigned int width:8;
	unsigned int bold:1;
	unsigned int ralign:1;
	unsigned int frame:1;
	char *text;
	char payload[];
};

/**
 * @brief Sets the label text, may trigger application resize.
 *
 * If the strings passed to the label are allocated the function is supposed to
 * be used as free(gp_widget_label_set(widget, strdup(string)).
 *
 * @param self Widget label pointer.
 * @param text New widget label text, the pointer is stored and the string is not copied.
 * @return Pointer to the previous label text.
 */
char *gp_widget_label_set(gp_widget *self, char *text);

/**
 * @brief Printf-like function to set label text.
 * @param self Pointer to label widget.
 * @param fmt  Printf formatting string.
 * @param ...  Printf parameters.
 * @return Number of characters printed.
 */
int gp_widget_label_printf(gp_widget *self, const char *fmt, ...)
                           __attribute__((format (printf, 2, 3)));

/**
 * @brief Turns on-off bold text attribute.
 */
static inline void gp_widget_label_bold(gp_widget *self, int bold)
{
	GP_WIDGET_ASSERT(self, GP_WIDGET_LABEL, );

	self->label->bold = bold;

	//TODO: do we need resize?
	gp_widget_redraw(self);
}

/**
 * @brief Allocates a label widget.
 *
 * If width is set, the text buffer is allocated to be width in size, otherwise
 * it's allocated to fit the text.
 *
 * @param text Widget label text, the string is copied with strdup().
 * @param width Maximal expected text width, if set this is used to callculate
 *              the label size, otherise the size fits only the label text.
 * @param bold Sets the bold text attribute.
 * @return Newly allocated label widget.
 */
gp_widget *gp_widget_label_new(const char *text, unsigned int width, int bold);

/**
 * @brief Printf-like function to create a label widget.
 *
 * @param bold Sets the bold text attribute.
 * @param fmt Printf formatting string.
 * @param ... Printf parameters.
 * @return Newly allocated label widget.
 */
gp_widget *gp_widget_label_printf_new(int bold, const char *fmt, ...)
                                      __attribute__((format (printf, 2, 3)));

#endif /* GP_WIDGET_LABEL_H__ */
