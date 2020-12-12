//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2020 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_TABS_H__
#define GP_WIDGET_TABS_H__

struct gp_widget_tabs {
	unsigned int active_tab;
	unsigned int count;

	int title_focused:1;
	int widget_focused:1;

	char **labels;
	struct gp_widget **widgets;

	char payload[];
};

gp_widget *gp_widget_tabs_new(unsigned int tabs, unsigned int active_tab,
                              const char *tab_labels[]);

gp_widget *gp_widget_tabs_put(gp_widget *self, unsigned int tab,
                              gp_widget *content);

#endif /* GP_WIDGET_TABS_H__ */
