/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef WEBKIT_ENABLED
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#endif
#include <gdk/gdk.h>

#include "help.h"

/*
 * Declarations
 */

#ifdef WEBKIT_ENABLED
static void
libgebr_gui_help_show_on_title_changed(WebKitWebView * web_view,
	WebKitWebFrame * frame, gchar * title, GtkWindow * window);
static GtkWidget *
libgebr_gui_help_show_create_web_view(void);
#endif

/*
 * Public functions
 */

void
libgebr_gui_help_show(const gchar * uri)
{
#ifdef WEBKIT_ENABLED
	GtkWidget *	web_view;

	web_view = libgebr_gui_help_show_create_web_view();
	webkit_web_view_open(WEBKIT_WEB_VIEW(web_view), uri);
#endif
}

/*
 * Private functions
 */

#ifdef WEBKIT_ENABLED
static GtkWidget *
libgebr_gui_help_show_create_web_view(void)
{
	static GtkWindowGroup *	window_group = NULL;
	static GtkWidget *	work_around_web_view = NULL;
	GtkWidget *		window;
	GtkWidget *		scrolled_window;
	GtkWidget *		web_view;

	if (window_group == NULL)
		window_group = gtk_window_group_new();
	if (!g_thread_supported())
		g_thread_init(NULL);
	/* WORKAROUND: Newer WebKitGtk versions crash on last WebKitWebView destroy,
	so we always keep one instance of it. */
	if (work_around_web_view == NULL)
		work_around_web_view = webkit_web_view_new();

	window = gtk_dialog_new();
	gtk_window_group_add_window(window_group, GTK_WINDOW(window));
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	web_view = webkit_web_view_new();
	if (g_signal_lookup("create-web-view", WEBKIT_TYPE_WEB_VIEW))
		g_signal_connect(web_view, "create-web-view",
			G_CALLBACK(libgebr_gui_help_show_create_web_view), NULL);
	g_signal_connect(web_view, "title-changed",
		G_CALLBACK(libgebr_gui_help_show_on_title_changed), window);

	/* Place the WebKitWebView in the GtkScrolledWindow */
	gtk_container_add(GTK_CONTAINER (scrolled_window), web_view);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), scrolled_window, TRUE, TRUE, 0);

	/* Show the result */
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
	gtk_widget_show_all(window);

	return web_view;
}

static void
libgebr_gui_help_show_on_title_changed(WebKitWebView * web_view,
	WebKitWebFrame * frame, gchar * title, GtkWindow * window)
{
	gtk_window_set_title(window, title);
}

#endif //WEBKIT_ENABLED
