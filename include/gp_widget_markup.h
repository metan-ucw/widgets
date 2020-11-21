//SPDX-License-Identifier: LGPL-2.0-or-later
/*

   Copyright (c) 2014-2020 Cyril Hrubis <metan@ucw.cz>

 */

/*

   Simple markup.

   - variables are enclosed in {0.000}
     the string between {} is used for
     initial size computation

   - newlines are encoded with \n

   - bold is enclosed with asterisks: *bold*

   - escape character is \ as in \* or \{

 */

#ifndef GP_WIDGET_MARKUP_H__
#define GP_WIDGET_MARKUP_H__

struct gp_widget_markup {
	char *text;
	char *(*get)(unsigned int var_id, char *old_val);
	unsigned int lines;
	unsigned int cnt;
	char payload[];
};

gp_widget *gp_widget_markup_new(const char *markup,
                                char *(*get)(unsigned int var_id, char *old_val));


/*
 */
void gp_widget_markup_set_var(gp_widget *self, unsigned int var_id, const char *fmt, ...);

/*
 * Causes variables to be updated.
 */
void gp_widget_markup_refresh(gp_widget *self);

#endif /* GP_WIDGET_MARKUP_H__ */
