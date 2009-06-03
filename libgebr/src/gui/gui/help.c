/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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
#include <webkit/webkit.h>
#endif
#include <gdk/gdk.h>

#include "help.h"

/*
 * Declarations
 */

#ifdef WEBKIT_ENABLED
static WebKitNavigationResponse
libgebr_gui_help_show_navigation_resquested(WebKitWebView * web_view, WebKitWebFrame * frame,
	WebKitNetworkRequest * request);
static GtkWidget *
libgebr_gui_help_show_create_web_view(gboolean filter_links);
#endif

/*
 * Public functions
 */

void
libgebr_gui_help_show(const gchar * help)
{
#ifdef WEBKIT_ENABLED
	GtkWidget *	web_view;

	web_view = libgebr_gui_help_show_create_web_view(TRUE);
	webkit_web_view_load_html_string(WEBKIT_WEB_VIEW(web_view), help, NULL);
#endif
}

/*
 * Private functions
 */

#ifdef WEBKIT_ENABLED
static GtkWidget *
libgebr_gui_help_show_create_web_view(gboolean filter_links)
{
	static GtkWindowGroup *	window_group = NULL;
	GtkWidget *		window;
	GtkWidget *		scrolled_window;
	GtkWidget *		web_view;

	if (window_group == NULL)
		window_group = gtk_window_group_new();
	if (!g_thread_supported())
		g_thread_init(NULL);

	window = gtk_dialog_new();
	gtk_window_group_add_window(window_group, GTK_WINDOW(window));
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	web_view = webkit_web_view_new();
	if (filter_links)
		g_signal_connect(web_view, "navigation-requested",
			G_CALLBACK(libgebr_gui_help_show_navigation_resquested), NULL);

	/* Place the WebKitWebView in the GtkScrolledWindow */
	gtk_container_add(GTK_CONTAINER (scrolled_window), web_view);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), scrolled_window, TRUE, TRUE, 0);

	/* Show the result */
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
	gtk_widget_show_all(window);

	return web_view;
}

static WebKitNavigationResponse
libgebr_gui_help_show_navigation_resquested(WebKitWebView * web_view, WebKitWebFrame * frame,
	WebKitNetworkRequest * request)
{
	const gchar *	uri;
	GError *	error = NULL;

	uri = webkit_network_request_get_uri(request);
	if (g_str_has_prefix(uri, "#"))
		return WEBKIT_NAVIGATION_RESPONSE_ACCEPT;

#ifdef GTK_CHECK_VERSION(2,14,0)
	gtk_show_uri(gdk_screen_get_default(), uri, GDK_CURRENT_TIME, &error);
#else
	GtkWidget *	new_web_view;

	new_web_view = libgebr_gui_help_show_create_web_view(FALSE);
	webkit_web_view_open(WEBKIT_WEB_VIEW(new_web_view), uri);
#endif
	
	return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
}
#endif //WEBKIT_ENABLED
