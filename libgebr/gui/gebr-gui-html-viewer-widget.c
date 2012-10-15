/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebr_guiproject/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or * (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see
 *   <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "../libgebr-gettext.h"

#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <geoxml.h>
#include <regex.h>
#include <string.h>
#include <webkit/webkit.h>

#include "gebr-gui-html-viewer-widget.h"
#include "../utils.h"
#include "gebr-gui-utils.h"
#include "gebr-gui-js.h"

#define CSS_LINK "<link rel=\"stylesheet\" type=\"text/css\" href=\"file://"LIBGEBR_DATA_DIR"/gebr.css\" />"

enum {
	TITLE_READY,
	PRINT_REQUESTED,
	LAST_SIGNAL
};

static guint signals[ LAST_SIGNAL ] = { 0 };

typedef struct _GebrGuiHtmlViewerWidgetPrivate GebrGuiHtmlViewerWidgetPrivate;

struct _GebrGuiHtmlViewerWidgetPrivate {
	GtkWidget * web_view;
	GebrGeoXmlObject *object;
	gchar *tmp_file;
	GebrGuiHtmlViewerCustomTab callback;
	gchar * label;
	GString *content;
	gboolean is_menu;

	// For search bar
	GtkWidget *find;
	GtkWidget *find_entry;
};

#define GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE((o), GEBR_GUI_TYPE_HTML_VIEWER_WIDGET, GebrGuiHtmlViewerWidgetPrivate))

//==============================================================================
// PROTOTYPES								       =
//==============================================================================

static void on_load_finished(WebKitWebView * web_view, WebKitWebFrame * frame, GebrGuiHtmlViewerWidget *self);

static WebKitNavigationResponse on_navigation_requested(WebKitWebView * web_view, WebKitWebFrame *frame,
							WebKitNetworkRequest *request, GebrGuiHtmlViewerWidget *self);

static void on_search_close_button(GtkButton *button, GebrGuiHtmlViewerWidget *self);

static gboolean on_web_view_search_key_press(GtkWidget *widget, GdkEventKey *key,
                                             GebrGuiHtmlViewerWidget *self);

static gboolean on_web_view_search_escape(GtkWidget *widget, GdkEventKey *key,
                                          GebrGuiHtmlViewerWidget *self);

static void gebr_gui_html_viewer_widget_search (GebrGuiHtmlViewerWidget *self,
                                                const gchar *text,
                                                gboolean backward);

static void on_web_view_search_backward(GtkButton *button, GebrGuiHtmlViewerWidget *self);

static void on_web_view_search_foward(GtkButton *button, GebrGuiHtmlViewerWidget *self);

static void on_web_view_search(GtkWidget *entry, GebrGuiHtmlViewerWidget *self);

static void gebr_gui_html_viewer_widget_finalize (GObject *object);

G_DEFINE_TYPE(GebrGuiHtmlViewerWidget, gebr_gui_html_viewer_widget, GTK_TYPE_VBOX);

//==============================================================================
// GOBJECT RELATED FUNCTIONS						       =
//==============================================================================

static void gebr_gui_html_viewer_widget_class_init(GebrGuiHtmlViewerWidgetClass * klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = gebr_gui_html_viewer_widget_finalize;

	/**
	 * GebrGuiHtmlViewerWidget::title-ready:
	 * @widget: This #GebrGuiHtmlViewerWidget
	 * @title: The title.
	 *
	 * This signal is fired when the title is ready to be set. The title is passed by parameter @title, and its
	 * value may depend on context. If this viewer is showing the help of a geoxml-menu, the title is the same as
	 * the menu. Otherwise, the title is defined by the &lt;title&gt; tag inside the Html.
	 */
	signals[ TITLE_READY ] =
		g_signal_new("title-ready",
			     GEBR_GUI_TYPE_HTML_VIEWER_WIDGET,
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrGuiHtmlViewerWidgetClass, title_ready),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__STRING,
			     G_TYPE_NONE,
			     1,
			     G_TYPE_STRING);

	/**
	 * GebrGuiHtmlViewerWidget::print-requested:
	 * Emitted when the user requests a print job.
	 */
	signals[ PRINT_REQUESTED ] =
		g_signal_new("print-requested",
			     GEBR_GUI_TYPE_HTML_VIEWER_WIDGET,
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrGuiHtmlViewerWidgetClass, print_requested),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__VOID,
			     G_TYPE_NONE, 0);

	g_type_class_add_private(klass, sizeof(GebrGuiHtmlViewerWidgetPrivate));
}

static void gebr_gui_html_viewer_widget_init(GebrGuiHtmlViewerWidget * self)
{
	GString *tmp_file;
	GtkWidget * scrolled_window;
	GebrGuiHtmlViewerWidgetPrivate * priv;

	priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);
	priv->object = NULL;
	priv->web_view = webkit_web_view_new();
	priv->callback = NULL;
	priv->content = g_string_new (NULL);
	tmp_file = gebr_make_temp_filename("XXXXXX.html");
	priv->tmp_file = g_string_free (tmp_file, FALSE);
	priv->is_menu = FALSE;

	/*
	 * Create search bar
	 */
	priv->find = gtk_hbox_new(FALSE, 5);
	g_signal_connect(priv->find, "key-press-event", G_CALLBACK(on_web_view_search_escape), self);

	GtkWidget *label = gtk_label_new(_("Search:"));
	gtk_widget_show(label);

	priv->find_entry = gtk_entry_new();
	gtk_widget_show(priv->find_entry);
	g_signal_connect(priv->find_entry, "changed", G_CALLBACK(on_web_view_search), self);
	g_signal_connect(priv->find_entry, "key-press-event", G_CALLBACK(on_web_view_search_key_press), self);

	GtkWidget *arrow_up = gtk_button_new();
	gtk_button_set_relief(GTK_BUTTON(arrow_up), GTK_RELIEF_NONE);
	gtk_container_add(GTK_CONTAINER(arrow_up), gtk_arrow_new(GTK_ARROW_UP, GTK_SHADOW_NONE));
	g_signal_connect(arrow_up, "clicked", G_CALLBACK(on_web_view_search_backward), self);
	gtk_widget_show_all(arrow_up);

	GtkWidget *arrow_down = gtk_button_new();
	gtk_button_set_relief(GTK_BUTTON(arrow_down), GTK_RELIEF_NONE);
	gtk_container_add(GTK_CONTAINER(arrow_down), gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_NONE));
	g_signal_connect(arrow_down, "clicked", G_CALLBACK(on_web_view_search_foward), self);
	gtk_widget_show_all(arrow_down);

	GtkWidget *close_button = gtk_button_new();
	gtk_button_set_relief(GTK_BUTTON(close_button), GTK_RELIEF_NONE);
	GtkWidget *image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(close_button), image);
	g_signal_connect(close_button, "clicked", G_CALLBACK(on_search_close_button), self);
	gtk_widget_show_all(close_button);

	gtk_box_pack_start(GTK_BOX(priv->find), label, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(priv->find), priv->find_entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(priv->find), arrow_up, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(priv->find), arrow_down, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(priv->find), close_button, FALSE, FALSE, 0);
	gtk_widget_set_no_show_all(priv->find, TRUE);
	/*
	 * End of search bar
	 */

	g_signal_connect(priv->web_view, "load-finished", G_CALLBACK(on_load_finished), self);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scrolled_window), priv->web_view);
	gtk_box_pack_start(GTK_BOX(self), scrolled_window, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(self), priv->find, FALSE, FALSE, 5);
	gtk_widget_show_all(scrolled_window);
}

static void gebr_gui_html_viewer_widget_finalize(GObject *object)
{
	GebrGuiHtmlViewerWidgetPrivate * priv;
	priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(object);
	
	g_unlink (priv->tmp_file);
	g_free (priv->tmp_file);

	g_free(priv->label);
	g_string_free (priv->content, TRUE);

	G_OBJECT_CLASS (gebr_gui_html_viewer_widget_parent_class)->finalize (object);
}

//==============================================================================
// PRIVATE FUNCTIONS							       =
//==============================================================================
static void on_search_close_button(GtkButton *button,
                                   GebrGuiHtmlViewerWidget *self)
{
	GebrGuiHtmlViewerWidgetPrivate * priv;
	priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);

	g_return_if_fail(GEBR_GUI_IS_HTML_VIEWER_WIDGET(self));

	gtk_widget_hide(priv->find);
}

static void gebr_gui_html_viewer_widget_search (GebrGuiHtmlViewerWidget *self,
                                                const gchar *text,
                                                gboolean backward)
{
	GebrGuiHtmlViewerWidgetPrivate * priv;
	priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);

	g_return_if_fail(GEBR_GUI_IS_HTML_VIEWER_WIDGET(self));

	webkit_web_view_search_text(WEBKIT_WEB_VIEW(priv->web_view), text, FALSE, !backward, TRUE);
}

static void on_web_view_search_backward(GtkButton *button,
                                        GebrGuiHtmlViewerWidget *self)
{
	GebrGuiHtmlViewerWidgetPrivate * priv;
	priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);

	g_return_if_fail(GEBR_GUI_IS_HTML_VIEWER_WIDGET(self));

	gebr_gui_html_viewer_widget_search(self, gtk_entry_get_text(GTK_ENTRY(priv->find_entry)), TRUE);
}

static void on_web_view_search_foward(GtkButton *button,
                                      GebrGuiHtmlViewerWidget *self)
{
	GebrGuiHtmlViewerWidgetPrivate * priv;
	priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);

	g_return_if_fail(GEBR_GUI_IS_HTML_VIEWER_WIDGET(self));

	gebr_gui_html_viewer_widget_search(self, gtk_entry_get_text(GTK_ENTRY(priv->find_entry)), FALSE);
}

static gboolean on_web_view_search_key_press(GtkWidget *widget,
                                             GdkEventKey *key,
                                             GebrGuiHtmlViewerWidget *self)
{
	GebrGuiHtmlViewerWidgetPrivate * priv;
	priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);

	if (key->keyval == GDK_KP_Enter ||
	    key->keyval == GDK_Return) {
		gebr_gui_html_viewer_widget_search(self, gtk_entry_get_text(GTK_ENTRY(priv->find_entry)),
		                                   key->state & GDK_SHIFT_MASK);
		return TRUE;
	}

	return FALSE;
}

static gboolean on_web_view_search_escape(GtkWidget *widget,
                                          GdkEventKey *key,
                                          GebrGuiHtmlViewerWidget *self)
{
	if (key->keyval != GDK_Escape)
		return FALSE;

	on_search_close_button(NULL, self);
	return TRUE;
}

static void on_web_view_search(GtkWidget *entry,
                               GebrGuiHtmlViewerWidget *self)
{
	g_return_if_fail(GEBR_GUI_IS_HTML_VIEWER_WIDGET(self));

	gebr_gui_html_viewer_widget_search(self, gtk_entry_get_text(GTK_ENTRY(entry)), FALSE);
}

static void create_generate_links_index_function(JSContextRef context)
{
	static const gchar * script =
		"function GenerateLinksIndex(links, is_programs_help) {"
			"var navbar = document.getElementsByClassName('navigation')[0];"
			"if (!navbar) return;"
			"var linklist = navbar.getElementsByTagName('ul')[0];"
			"if (!linklist) return;"
			"if (!is_programs_help) {"
				"var session = document.createElement('li');"
				"session.innerHTML = '<h2>Programs</h2>';"
				"linklist.appendChild(session);"
			"}"
			"for (var i = 0; i < links.length; ++i) {"
				"var link = document.createElement('a');"
				"link.setAttribute('href', links[i][1]);"
				"link.innerHTML = links[i][0];"
				"var li = document.createElement('li');"
				"li.appendChild(link);"
				"linklist.appendChild(li);"
			"}"
		"}"
		"function generateNavigationIndex() {"
			"var navbar = document.getElementsByClassName('navigation')[0];"
			"var content = document.getElementsByClassName('content')[0];"
			"if (!navbar || !content) return;"
			"var headers = content.getElementsByTagName('h2');"
			"navbar.innerHTML = '<h2>Index</h2><ul></ul>';"
			"var navlist = navbar.getElementsByTagName('ul')[0];"
			"for (var i = 0; i < headers.length; i++) {"
				"var anchor = 'header_' + i;"
				"var link = document.createElement('a');"
				"link.setAttribute('href', '#' + anchor);"
				"for (var j = 0; j < headers[i].childNodes.length; j++) {"
					"var clone = headers[i].childNodes[j].cloneNode(true);"
					"link.appendChild(clone);"
				"}"
				"var li = document.createElement('li');"
				"li.appendChild(link);"
				"navlist.appendChild(li);"
				"headers[i].setAttribute('id', anchor);"
			"}"
		"}";

	gebr_js_evaluate(context, script);
}

static void generate_links_index(JSContextRef context,
                                 const gchar * links,
                                 gboolean is_programs_help)
{
	gchar * script;

	if (links) {
		const gchar * program;
		program = is_programs_help? "true":"false";
		script = g_strdup_printf("generateNavigationIndex(); GenerateLinksIndex(%s,%s);", links, program);
	} else {
		script = g_strdup("generateNavigationIndex();");
	}

	gebr_js_evaluate(context, script);
	g_free(script);
}

static gchar * js_get_document_title(JSContextRef context)
{
	JSValueRef value;
	GString * title;
	value = gebr_js_evaluate(context, "document.title");
	title = gebr_js_value_get_string(context, value);
	return g_string_free(title, FALSE);
}

static void on_load_finished(WebKitWebView * web_view, WebKitWebFrame * frame, GebrGuiHtmlViewerWidget *self)
{
	GebrGuiHtmlViewerWidgetPrivate * priv;
	GebrGeoXmlObjectType type;
	JSContextRef context;
	gchar * title;

	priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);
	context = webkit_web_frame_get_global_context(frame);

	if (priv->object) {
		gchar *help;

		type = gebr_geoxml_object_get_type(priv->object);
		create_generate_links_index_function(context);

		if (type == GEBR_GEOXML_OBJECT_TYPE_PROGRAM) {
			GebrGeoXmlDocument *menu = gebr_geoxml_object_get_owner_document(priv->object);
			const gchar *uri = gebr_geoxml_program_get_url(GEBR_GEOXML_PROGRAM(priv->object));
			help = gebr_geoxml_document_get_help (menu);

			if (priv->is_menu && strlen(uri) > 0) {
				generate_links_index(context, "[['<b>Menu</b>', 'gebr://menu'], ['<b>More Info</b>', 'gebr://link']]", TRUE);
			} else if(priv->is_menu)
				generate_links_index(context, "[['<b>Menu</b>', 'gebr://menu']]", TRUE);
			else if (strlen(uri) > 0)
				generate_links_index(context, "[['<b>More Info</b>', 'gebr://link']]", TRUE);
			else
				generate_links_index(context, NULL, TRUE);

			g_free (help);
		} else if (type == GEBR_GEOXML_OBJECT_TYPE_FLOW) {
			GString * list;
			GebrGeoXmlSequence *program;

			list = g_string_new("[");
			gebr_geoxml_flow_get_program(GEBR_GEOXML_FLOW(priv->object), &program, 0);

			for (gint i = 0; program != NULL; gebr_geoxml_sequence_next(&program), ++i) {
				help = gebr_geoxml_program_get_help (GEBR_GEOXML_PROGRAM (program));
				if(strlen(help) > 0)
					g_string_append_printf(list, "%s['%s', 'gebr://prog%d']",
						       	   i == 0? "":",",
							   gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(program)), i);
				g_free (help);
			}
			g_string_append(list, "]");
			if(strcmp(list->str,"[]") != 0)
				generate_links_index(context, list->str, FALSE);
			else
				generate_links_index(context, NULL, FALSE);
			g_string_free(list, TRUE);
		}
	} else {
		create_generate_links_index_function(context);
		generate_links_index(context, NULL, FALSE);
	}

	title = js_get_document_title(context);

	g_signal_connect(priv->web_view, "navigation-requested", G_CALLBACK(on_navigation_requested), self);
	g_signal_emit(self, signals[ TITLE_READY ], 0, title);
	g_free (title);
}

static WebKitNavigationResponse on_navigation_requested(WebKitWebView * web_view, WebKitWebFrame *frame,
							WebKitNetworkRequest *request, GebrGuiHtmlViewerWidget *self)
{
	GebrGuiHtmlViewerWidgetPrivate * priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);
	const gchar * uri = webkit_network_request_get_uri(request);

	if (priv->object && g_str_has_prefix(uri, "gebr://")) {
		GebrGeoXmlObject *object;
		if (!strcmp(uri, "gebr://menu")) {
			if (gebr_geoxml_object_get_type(priv->object) == GEBR_GEOXML_OBJECT_TYPE_FLOW) {
				gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, NULL,
							_("Invalid link"), _("Sorry, couldn't reach link."));
				return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
			}
			GebrGeoXmlDocument *menu = gebr_geoxml_object_get_owner_document(priv->object);
			object = GEBR_GEOXML_OBJECT(menu);
		} else if (!g_strcmp0(uri, "gebr://link")) {
			if (gebr_geoxml_object_get_type(priv->object) == GEBR_GEOXML_OBJECT_TYPE_FLOW) {
				gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, NULL,
				                        _("Invalid link"), _("Sorry, couldn't reach link."));
				return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
			}
			GString *full_uri = g_string_new(NULL);
			const gchar *url = gebr_geoxml_program_get_url(GEBR_GEOXML_PROGRAM(priv->object));
			if (g_str_has_prefix(url, "http://"))
				g_string_assign(full_uri, url);
			else
				g_string_printf(full_uri, "http://%s", uri);

			gebr_gui_show_uri(full_uri->str);

			g_string_free(full_uri, TRUE);

			return  WEBKIT_NAVIGATION_RESPONSE_IGNORE;
		} else {
			if (gebr_geoxml_object_get_type(priv->object) != GEBR_GEOXML_OBJECT_TYPE_FLOW) {
				gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, NULL,
							_("Invalid link"), _("Sorry, couldn't reach link."));
				return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
			}
			int program_index = -1;
			sscanf(strstr(uri, "prog"), "prog%d", &program_index);
			GebrGeoXmlSequence *program;
			gebr_geoxml_flow_get_program(GEBR_GEOXML_FLOW(priv->object), &program, program_index);
			if (program == NULL) {
				gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, NULL,
							_("Invalid link"), _("Sorry, couldn't find program."));
				return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
			}

			object = GEBR_GEOXML_OBJECT(program);
		}

		gchar *help;
		GebrGeoXmlObjectType type;
		type = gebr_geoxml_object_get_type(object);

		if (type == GEBR_GEOXML_OBJECT_TYPE_FLOW)
			help = gebr_geoxml_document_get_help (GEBR_GEOXML_DOCUMENT (object));
		else
			help = gebr_geoxml_program_get_help (GEBR_GEOXML_PROGRAM (object));

		if(strlen(help) == 0) {
			gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, NULL,
						_("Invalid link"), _("Sorry, couldn't reach link."));
			g_free (help);
			return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
		}

		gebr_gui_html_viewer_widget_generate_links(self, object);
		gebr_gui_html_viewer_widget_show_html(self, help);
		g_free (help);

		return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
	}

	if (g_str_has_prefix(uri + strlen("file://"), priv->tmp_file) || g_str_has_prefix(uri, "about:"))
		return WEBKIT_NAVIGATION_RESPONSE_ACCEPT;

	gebr_gui_show_uri(uri);

	return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
}

static void pre_process_html(GebrGuiHtmlViewerWidgetPrivate * priv)
{
	regex_t regexp;
	regmatch_t matchptr;

	if (!priv->content->len)
		return;

	regcomp(&regexp, "<link[^<]*gebr.css[^<]*>", REG_NEWLINE | REG_ICASE);
	if (!regexec(&regexp, priv->content->str, 1, &matchptr, 0)) {
		/*
		 * If we found a link for GeBR's style sheet, we must update its path.
		 */
		gssize start = matchptr.rm_so;
		gssize length = matchptr.rm_eo - matchptr.rm_so;
		g_string_erase(priv->content, start, length);
		g_string_insert(priv->content, start, CSS_LINK);
	} else if (priv->object != NULL) {
		/*
		 * Otherwise, we only include the CSS link if we have a
		 * GeoXml object associated with this priv->content view, because
		 * this mean we are viewing a Flow or Program.
		 */
		regcomp(&regexp, "<head>", REG_NEWLINE | REG_ICASE);
		if (!regexec(&regexp, priv->content->str, 1, &matchptr, 0)) {
			gssize start = matchptr.rm_eo;
			g_string_insert(priv->content, start, CSS_LINK);
		}
	}
}

static GtkWidget * on_create_custom_widget(GtkPrintOperation * print_op, GebrGuiHtmlViewerWidget *self)
{
	GebrGuiHtmlViewerWidgetPrivate * priv;
	priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);

	if (priv->callback)
		return priv->callback(self); 

	return NULL;
}

static void on_apply_custom_widget(GtkPrintOperation * print_op, GtkWidget * widget, GebrGuiHtmlViewerWidget * self)
{
	g_signal_emit(self, signals[ PRINT_REQUESTED ], 0);
}

//==============================================================================
// PUBLIC FUNCTIONS							       =
//==============================================================================

GtkWidget *gebr_gui_html_viewer_widget_new(void)
{
	return  g_object_new(GEBR_GUI_TYPE_HTML_VIEWER_WIDGET, NULL);
}

void gebr_gui_html_viewer_show_search_bar(GebrGuiHtmlViewerWidget *self)
{
	GebrGuiHtmlViewerWidgetPrivate * priv;
	priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);

	g_return_if_fail(GEBR_GUI_IS_HTML_VIEWER_WIDGET(self));

	if (gtk_widget_get_visible(priv->find)) {
		on_web_view_search_foward(NULL, self);
	} else {
		gtk_widget_grab_focus(priv->find_entry);
		gtk_widget_show(priv->find);
	}
}

void gebr_gui_html_viewer_widget_print(GebrGuiHtmlViewerWidget * self)
{
	GebrGuiHtmlViewerWidgetPrivate * priv;
	priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);

	g_return_if_fail(GEBR_GUI_IS_HTML_VIEWER_WIDGET(self));

#if WEBKIT_CHECK_VERSION(1,1,5)
	WebKitWebFrame * frame;
	GtkPrintOperation * print_op;
	GError * error = NULL;

	print_op = gtk_print_operation_new();
	g_signal_connect(print_op, "create-custom-widget", G_CALLBACK(on_create_custom_widget), self);
	g_signal_connect(print_op, "custom-widget-apply", G_CALLBACK(on_apply_custom_widget), self);
	gtk_print_operation_set_custom_tab_label(print_op, priv->label);

	frame = webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(priv->web_view));
	webkit_web_frame_print_full(frame, print_op, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, &error);

	if (error) {
		g_warning("%s", error->message);
		g_clear_error(&error);
	}
	g_object_unref(print_op);
#else
	gebr_gui_show_uri (priv->tmp_file);
#endif
}

void gebr_gui_html_viewer_widget_set_is_menu(GebrGuiHtmlViewerWidget * self,
                                             gboolean is_menu)
{
	GebrGuiHtmlViewerWidgetPrivate * priv;

	g_return_if_fail(GEBR_GUI_IS_HTML_VIEWER_WIDGET(self));

	priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);

	priv->is_menu = is_menu;
}

void gebr_gui_html_viewer_widget_show_html(GebrGuiHtmlViewerWidget * self, const gchar * content)
{
	GebrGuiHtmlViewerWidgetPrivate * priv;

	g_return_if_fail(GEBR_GUI_IS_HTML_VIEWER_WIDGET(self));

	priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);

	g_string_assign (priv->content, content);

	pre_process_html (priv);

	/* some webkit versions crash to open an empty file... */
	if (!priv->content->len)
		g_string_assign(priv->content, " ");

	/* write current priv->content to temporary file */
	FILE *fp;
	fp = fopen(priv->tmp_file, "w");
	fputs(priv->content->str, fp);
	fclose(fp);

	webkit_web_view_open(WEBKIT_WEB_VIEW(priv->web_view), priv->tmp_file);
}

void gebr_gui_html_viewer_widget_generate_links(GebrGuiHtmlViewerWidget *self, GebrGeoXmlObject * object)
{
	g_return_if_fail(GEBR_GUI_IS_HTML_VIEWER_WIDGET(self));

	GebrGuiHtmlViewerWidgetPrivate * priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);
	priv->object = object;
}

void gebr_gui_html_viewer_widget_set_custom_tab (GebrGuiHtmlViewerWidget *self,
						 const gchar *label,
						 GebrGuiHtmlViewerCustomTab callback)
{
	g_return_if_fail(GEBR_GUI_IS_HTML_VIEWER_WIDGET(self));

	GebrGuiHtmlViewerWidgetPrivate * priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);
	priv->callback = callback;
	priv->label = g_strdup(label);
}

const gchar * gebr_gui_html_viewer_widget_get_html (GebrGuiHtmlViewerWidget *self)
{
	GebrGuiHtmlViewerWidgetPrivate *priv;

	g_return_val_if_fail (GEBR_GUI_IS_HTML_VIEWER_WIDGET (self), NULL);

	priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE (self);
	return priv->content->str;
}

GebrGeoXmlObject *
gebr_gui_html_viewer_widget_get_related_object (GebrGuiHtmlViewerWidget *self)
{
	g_return_val_if_fail (GEBR_GUI_IS_HTML_VIEWER_WIDGET (self), NULL);

	GebrGuiHtmlViewerWidgetPrivate * priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE (self);
	return priv->object;
}

void
gebr_gui_html_viewer_widget_load_anchor (GebrGuiHtmlViewerWidget *self,
                                         gint anchor)
{
	GebrGuiHtmlViewerWidgetPrivate * priv;

	g_return_if_fail(GEBR_GUI_IS_HTML_VIEWER_WIDGET(self));

	priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);

	const gchar *uri = webkit_web_view_get_uri(WEBKIT_WEB_VIEW(priv->web_view));
	gchar *new_uri;

	if (!uri)
		return;

	gchar *find = g_strrstr(uri, "#");
	if (find) {
		if (atoi(find+1) == anchor)
			return;
		find[0] = '\0';
	}
	else if (anchor == -1)
		return;

	if (anchor >= 0)
		new_uri = g_strdup_printf("%s#%d", uri, anchor);
	else
		new_uri = g_strdup(uri);

	webkit_web_view_load_uri(WEBKIT_WEB_VIEW(priv->web_view), new_uri);

	g_free(new_uri);
}
