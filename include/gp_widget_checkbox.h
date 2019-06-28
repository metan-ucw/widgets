//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_CHECKBOX_H__
#define GP_WIDGET_CHECKBOX_H__

#include <gp_widget_bool.h>

struct gp_widget *gp_widget_checkbox_new(const char *label,
                                         int set,
                                         int (*on_event)(gp_widget_event *),
                                         void *priv);

#endif /* GP_WIDGET_CHECKBOX_H__ */
