#include <gfxprim.h>
#include <gp_widgets.h>

static void *uids;

int login_callback(gp_widget_event *ev)
{
	if (ev->type != GP_WIDGET_EVENT_ACTION)
		return 0;

	gp_widget *pass = gp_widget_by_uid(uids, "pass", GP_WIDGET_TEXTBOX);
	gp_widget *uname = gp_widget_by_uid(uids, "uname", GP_WIDGET_TEXTBOX);

	if (uname)
		printf("Username: '%s'\n", uname->tbox->buf);

	if (pass)
		printf("Password: '%s'\n", pass->tbox->buf);

	return 0;
}

int main(void)
{
	gp_widget *layout = gp_widget_layout_json("layout.json", &uids);
	if (!layout) {
		fprintf(stderr, "Failed to load layout!\n");
		return 1;
	}

	gp_widgets_main_loop(layout, "Login!", NULL);

	return 0;
}
