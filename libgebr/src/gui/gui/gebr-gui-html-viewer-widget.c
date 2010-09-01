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

#include <string.h>
#include <regex.h>
#include <webkit/webkit.h>
#include <glib.h>
#include <geoxml.h>
#include "gebr-gui-html-viewer-widget.h"
#include "../../intl.h"
#include "../../utils.h"
#include "../../defines.h"
#include "utils.h"
#include "gebr-gui-js.h"

#define CSS_LINK "<link rel=\"stylesheet\" type=\"text/css\" href=\"file://"LIBGEBR_DATA_DIR"/gebr.css\" />"

enum {
	PROP_0
};

typedef struct _GebrGuiHtmlViewerWidgetPrivate GebrGuiHtmlViewerWidgetPrivate;

struct _GebrGuiHtmlViewerWidgetPrivate {
	GtkWidget * web_view;
	GebrGeoXmlObject *object;
	gboolean generate_links;
};

#define GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE((o), GEBR_GUI_TYPE_HTML_VIEWER_WIDGET, GebrGuiHtmlViewerWidgetPrivate))

//==============================================================================
// PROTOTYPES								       =
//==============================================================================

static void gebr_gui_html_viewer_widget_set_property	(GObject	*object,
							 guint		 prop_id,
							 const GValue	*value,
							 GParamSpec	*pspec);
static void gebr_gui_html_viewer_widget_get_property	(GObject	*object,
							 guint		 prop_id,
							 GValue		*value,
							 GParamSpec	*pspec);
static void gebr_gui_html_viewer_widget_destroy(GtkObject *object);

static void on_load_finished(WebKitWebView * web_view, WebKitWebFrame * frame, GebrGuiHtmlViewerWidget *self);
static WebKitNavigationResponse on_navigation_requested(WebKitWebView * web_view, 
							WebKitWebFrame *frame,
							WebKitNetworkRequest *request,
							GebrGuiHtmlViewerWidget *self);

G_DEFINE_TYPE(GebrGuiHtmlViewerWidget, gebr_gui_html_viewer_widget, GTK_TYPE_VBOX);

//==============================================================================
// GOBJECT RELATED FUNCTIONS						       =
//==============================================================================

static void gebr_gui_html_viewer_widget_class_init(GebrGuiHtmlViewerWidgetClass * klass)
{
	GObjectClass *gobject_class;
	GtkObjectClass *object_class;

	gobject_class = G_OBJECT_CLASS(klass);
	object_class = GTK_OBJECT_CLASS(klass);
	gobject_class->set_property = gebr_gui_html_viewer_widget_set_property;
	gobject_class->get_property = gebr_gui_html_viewer_widget_get_property;
	object_class->destroy = gebr_gui_html_viewer_widget_destroy;

	g_type_class_add_private(klass, sizeof(GebrGuiHtmlViewerWidgetPrivate));
}

static void gebr_gui_html_viewer_widget_init(GebrGuiHtmlViewerWidget * self)
{
	GtkWidget * vbox;
	GtkWidget * scrolled_window;
	GebrGuiHtmlViewerWidgetPrivate * priv;

	priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);
	priv->object = NULL;
	priv->web_view = webkit_web_view_new();

	g_signal_connect(priv->web_view, "navigation-requested", G_CALLBACK(on_navigation_requested), self);
	g_signal_connect(priv->web_view, "load-finished", G_CALLBACK(on_load_finished), self);

	vbox = GTK_WIDGET(self);
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scrolled_window), priv->web_view);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
	gtk_widget_show_all(scrolled_window);
}

static void gebr_gui_html_viewer_widget_set_property(GObject		*object,
						     guint		 prop_id,
						     const GValue	*value,
						     GParamSpec		*pspec)
{
	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void gebr_gui_html_viewer_widget_get_property(GObject	*object,
						     guint	 prop_id,
						     GValue	*value,
						     GParamSpec	*pspec)
{
	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void gebr_gui_html_viewer_widget_destroy(GtkObject *object)
{
}

//==============================================================================
// PRIVATE FUNCTIONS							       =
//==============================================================================
static void on_load_finished(WebKitWebView * web_view, WebKitWebFrame * frame, GebrGuiHtmlViewerWidget *self)
{
	GebrGuiHtmlViewerWidgetPrivate * priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);
	GebrGeoXmlObject * object = priv->object;
	JSContextRef context = webkit_web_frame_get_global_context(frame);
	
	if (priv->generate_links) {
		GString *js = g_string_new(
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
			"}");
		if (gebr_geoxml_object_get_type(object) == GEBR_GEOXML_OBJECT_TYPE_PROGRAM) {
			g_string_append(js, "GenerateLinksIndex([['<b>Menu</b>', 'gebr://menu']], true);");
			gebr_js_evaluate(context, js->str);
		} else if (gebr_geoxml_object_get_type(object) == GEBR_GEOXML_OBJECT_TYPE_FLOW) {
			g_string_append(js, "GenerateLinksIndex([");
			GebrGeoXmlSequence *program;
			gebr_geoxml_flow_get_program(GEBR_GEOXML_FLOW(object), &program, 0);
			for (gint i = 0; program != NULL; gebr_geoxml_sequence_next(&program), ++i)
				g_string_append_printf(js, "%s['%s', 'gebr://prog%d']",
						       i == 0 ? "" : ", ",
						       gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(program)), i);
			g_string_append(js, "], false);");
			gebr_js_evaluate(context, js->str);
		}
		g_string_free(js, TRUE);
	}
}

static WebKitNavigationResponse on_navigation_requested(WebKitWebView * web_view, 
							WebKitWebFrame *frame,
							WebKitNetworkRequest *request,
							GebrGuiHtmlViewerWidget *self)
{
	GebrGuiHtmlViewerWidgetPrivate * priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);
	const gchar * uri = webkit_network_request_get_uri(request);

	if (priv->generate_links && g_str_has_prefix(uri, "gebr://")) {
		if (priv->object == NULL)
			return WEBKIT_NAVIGATION_RESPONSE_IGNORE;

		const gchar *help;
		GebrGeoXmlObject *object;
		if (!strcmp(uri, "gebr://menu")) {
			if (gebr_geoxml_object_get_type(priv->object) == GEBR_GEOXML_OBJECT_TYPE_FLOW) {
				gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
							_("Invalid link"), _("Sorry, couldn't reach link."));
				return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
			}
			GebrGeoXmlDocument *menu = gebr_geoxml_object_get_owner_document(priv->object);
			help = gebr_geoxml_document_get_help(menu);
			object = GEBR_GEOXML_OBJECT(menu);
		} else {
			if (gebr_geoxml_object_get_type(priv->object) != GEBR_GEOXML_OBJECT_TYPE_FLOW) {
				gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
							_("Invalid link"), _("Sorry, couldn't reach link."));
				return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
			}
			int program_index = -1;
			sscanf(strstr(uri, "prog"), "prog%d", &program_index);
			GebrGeoXmlSequence *program;
			gebr_geoxml_flow_get_program(GEBR_GEOXML_FLOW(priv->object), &program, program_index);
			if (program == NULL) {
				gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
							_("Invalid link"), _("Sorry, couldn't find program."));
				return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
			}

			help = gebr_geoxml_program_get_help(GEBR_GEOXML_PROGRAM(program));
			object = GEBR_GEOXML_OBJECT(program);
		}

		if (!strlen(help)) {
			gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
						_("No help available"), _("Sorry, the help is empty."));
			return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
		}

		gebr_gui_html_viewer_widget_set_geoxml_object(self, object);
		gebr_gui_html_viewer_widget_show_html(self, help);
		return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
	}
	if (g_str_has_prefix(uri, "file://") || g_str_has_prefix(uri, "about:"))
		return WEBKIT_NAVIGATION_RESPONSE_ACCEPT;

	gebr_gui_show_uri(uri);

	return WEBKIT_NAVIGATION_RESPONSE_IGNORE;

}

void pre_process_html(GebrGuiHtmlViewerWidgetPrivate * priv, GString *html)
{
	regex_t regexp;
	regmatch_t matchptr;

	if (!html->len)
		return;

	regcomp(&regexp, "<link[^<]*gebr.css[^<]*>", REG_NEWLINE | REG_ICASE);
	if (!regexec(&regexp, html->str, 1, &matchptr, 0)) {
		/*
		 * If we found a link for GeBR's style sheet, we must update its path.
		 */
		gssize start = matchptr.rm_so;
		gssize length = matchptr.rm_eo - matchptr.rm_so;
		g_string_erase(html, start, length);
		g_string_insert(html, start, CSS_LINK);
	} else if (priv->object != NULL) {
		/*
		 * Otherwise, we only include the CSS link if we have a
		 * GeoXml object associated with this html view, because
		 * this mean we are viewing a Flow or Program.
		 */
		regcomp(&regexp, "<head>", REG_NEWLINE | REG_ICASE);
		if (!regexec(&regexp, html->str, 1, &matchptr, 0)) {
			gssize start = matchptr.rm_eo;
			g_string_insert(html, start, CSS_LINK);
		}
	}
}

//==============================================================================
// PUBLIC FUNCTIONS							       =
//==============================================================================

GtkWidget *gebr_gui_html_viewer_widget_new(void)
{
	return  g_object_new(GEBR_GUI_TYPE_HTML_VIEWER_WIDGET, NULL);
}

void gebr_gui_html_viewer_widget_print(GebrGuiHtmlViewerWidget * self)
{
	GebrGuiHtmlViewerWidgetPrivate * priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);
	WebKitWebFrame * frame = webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(priv->web_view));
	webkit_web_frame_print(frame);
}
void gebr_gui_html_viewer_widget_show_html(GebrGuiHtmlViewerWidget * self, const gchar * content)
{

	GebrGuiHtmlViewerWidgetPrivate * priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);
	GString *_content = g_string_new(content);
	GString *tmp_file;

	tmp_file = gebr_make_temp_filename("XXXXXX.html");
	g_string_assign(_content, content);

	pre_process_html(priv, _content);

	/* some webkit versions crash to open an empty file... */
	if (!_content->len)
		g_string_assign(_content, " ");

	/* write current _content to temporary file */
	FILE *fp;
	fp = fopen(tmp_file->str, "w");
	fputs(_content->str, fp);
	fclose(fp);

	webkit_web_view_open(WEBKIT_WEB_VIEW(priv->web_view), tmp_file->str);

	g_string_free(tmp_file, TRUE);
	g_string_free(_content, TRUE);

}
void gebr_gui_html_viewer_widget_set_geoxml_object(GebrGuiHtmlViewerWidget *self, GebrGeoXmlObject * object)
{
	GebrGuiHtmlViewerWidgetPrivate * priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);

	priv->object = object;
}
void gebr_gui_html_viewer_widget_set_generate_links(GebrGuiHtmlViewerWidget *self, gboolean generate_links)
{
	GebrGuiHtmlViewerWidgetPrivate * priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);

	priv->generate_links = generate_links;
}
