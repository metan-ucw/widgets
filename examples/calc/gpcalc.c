//SPDX-License-Identifier: LGPL-2.0-or-later

/*

    Copyright (C) 2007-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <string.h>
#include <gfxprim.h>
#include <gp_widgets.h>

static void *uids;

/* calc state */
static double mem = 0;
static double val = 0;
static char op = 0;
static int op_flag = 0;

static double result(void)
{
	gp_widget *result = gp_widget_by_uid(uids, "result", GP_WIDGET_LABEL);

	return atof(result->label->text);
}

static void set_result(double res)
{
	gp_widget *result = gp_widget_by_uid(uids, "result", GP_WIDGET_LABEL);

	//TODO: trim zeroes!

	char buf[20];
	snprintf(buf, sizeof(buf), "%10lf", res);
	gp_widget_label_set(result, buf);
}

int m_clear(gp_widget_event *ev)
{
	if (ev->type != GP_WIDGET_EVENT_ACTION)
		return 0;

	mem = 0;

	return 0;
}

int m_plus(gp_widget_event *ev)
{
	if (ev->type != GP_WIDGET_EVENT_ACTION)
		return 0;

	mem += result();

	return 0;
}

int m_minus(gp_widget_event *ev)
{
	if (ev->type != GP_WIDGET_EVENT_ACTION)
		return 0;

	mem -= result();

	return 0;
}

int m_recall(gp_widget_event *ev)
{
	if (ev->type != GP_WIDGET_EVENT_ACTION)
		return 0;

	set_result(mem);

	return 0;
}

int clear(gp_widget_event *ev)
{
	if (ev->type != GP_WIDGET_EVENT_ACTION)
		return 0;

	gp_widget *result = gp_widget_by_uid(uids, "result", GP_WIDGET_LABEL);

	gp_widget_label_set(result, "0");
	op = 0;
	val = 0;

	return 0;
}

static void calc_val(void)
{
	gp_widget *result = gp_widget_by_uid(uids, "result", GP_WIDGET_LABEL);

	double tmp = atof(result->label->text);

	switch (op) {
	case '+':
		val += tmp;
	break;
	case '-':
		val -= tmp;
	break;
	case '*':
		val *= tmp;
	break;
	case '/':
		val /= tmp;
	break;
	}

	set_result(val);
}

static void store_val(void)
{
	gp_widget *result = gp_widget_by_uid(uids, "result", GP_WIDGET_LABEL);

	val = atof(result->label->text);
}

int do_op(gp_widget_event *ev)
{
	if (ev->type != GP_WIDGET_EVENT_ACTION)
		return 0;

	if (op)
		calc_val();
	else
		store_val();

	op_flag = 1;
	op = ev->self->btn->label[0];

	return 0;
}

int do_number(gp_widget_event *ev)
{
	if (ev->type != GP_WIDGET_EVENT_ACTION)
		return 0;

	gp_widget *result = gp_widget_by_uid(uids, "result", GP_WIDGET_LABEL);

	if (op_flag || !strcmp(result->label->text, "0")) {
		op_flag = 0;
		gp_widget_label_set(result, ev->self->btn->label);
		return 0;
	}

	char buf[20];

	snprintf(buf, sizeof(buf), "%s%s", result->label->text, ev->self->btn->label);
	gp_widget_label_set(result, buf);

	return 0;
}

int do_dec(gp_widget_event *ev)
{
	if (ev->type != GP_WIDGET_EVENT_ACTION)
		return 0;

	gp_widget *result = gp_widget_by_uid(uids, "result", GP_WIDGET_LABEL);

	if (strchr(result->label->text, '.'))
		return 0;

	return do_number(ev);
}

int main(void)
{
	gp_widget *layout = gp_widget_layout_json("gpcalc.json", &uids);

	gp_widgets_main_loop(layout, "gpcalc", NULL);

	return 0;
}
