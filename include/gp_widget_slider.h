//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2020 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_SLIDER_H__
#define GP_WIDGET_SLIDER_H__

gp_widget *gp_widget_slider_new(int min, int max, int val, int dir,
                                int (*on_event)(gp_widget_event *ev),
                                void *priv);

void gp_widget_slider_set(gp_widget *self, int val);


#endif /* GP_WIDGET_SLIDER_H__ */
