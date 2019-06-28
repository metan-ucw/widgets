//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

/**
 * Widget events is an interface between the application and the widget
 * toolkit, typical event is a button press or a text edit. Each widget usually
 * has only one event callback and sends a subset of event types.
 */

#ifndef GP_WIDGET_EVENT_H__
#define GP_WIDGET_EVENT_H__

#include <stdarg.h>

/**
 * @brief Widget event type.
 *
 * Determines the type of event, e.g. button press, text being edited, etc.
 */
enum gp_widget_event_type {
	/** Send right after widget has been allocated and initalized. */
	GP_WIDGET_EVENT_NEW,
	/** Commonly event for default widget action, e.g. button press. */
	GP_WIDGET_EVENT_ENTER,
	GP_WIDGET_EVENT_EDIT,
	GP_WIDGET_EVENT_FILTER,
	/** Raw user input event such as mouse movement or keypress. */
	GP_WIDGET_EVENT_INPUT,
	/** Send by pixmap widget when pixmap has has to be redrawn. */
	GP_WIDGET_EVENT_REDRAW,
	/** The number of events, i.e. last event + 1. */
	GP_WIDGET_EVENT_MAX,
};

/**
 * @brief Returns string name for a given event type.
 *
 * @param ev_type Widget event type.
 * @return Widget event type name.
 */
const char *gp_widget_event_type_name(enum gp_widget_event_type ev_type);

/**
 * @brief Event structure holds all event parameters.
 *
 *
 */
typedef struct gp_widget_event {
	struct gp_widget *self;
	void *priv;
	enum gp_widget_event_type type;
	union {
		void *ptr;
		long val;
		struct gp_event *input_ev;
	};
} gp_widget_event;

/**
 * @brief Prints event details into stdout.
 *
 * @param ev Pointer to a widget event.
 */
void gp_widget_event_dump(gp_widget_event *ev);

/**
 * @brief Helper function to send a widget event to application.
 *
 * @on_event Pointer to application event handler or NULL.
 * @self Pointer to the widget sending this event.
 * @priv Application private pointer, set by application on widget creation.
 * @type Event type see gp_widget_event_type enum.
 * @return The return value from application event handler.
 */
static inline int gp_widget_send_event(int (*on_event)(gp_widget_event *),
                                       struct gp_widget *self, void *priv,
				       enum gp_widget_event_type type, ...)
{
	if (!on_event)
		return 0;

	va_list va;
	va_start(va, type);
	long val = va_arg(va, long);
	va_end(va);

	gp_widget_event ev = {
		.self = self,
		.priv = priv,
		.type = type,
		.val = val,
	};

	return on_event(&ev);
}

#endif /* GP_WIDGET_EVENT_H__ */
