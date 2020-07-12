//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2020 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_BUTTON_H__
#define GP_WIDGET_BUTTON_H__

#include <gp_widget_bool.h>

struct gp_widget *gp_widget_button_new(const char *label,
                                       int (*on_event)(gp_widget_event *ev),
                                       void *priv);

#endif /* GP_WIDGET_BUTTON_H__ */
