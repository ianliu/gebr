/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or (at your option) any later version.
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

#include <webkit/webkit.h>
#include <libgebr/utils.h>

#include "gebr-help-edit.h"

/*
 * HTML_HOLDER:
 * Defines the HTML containing the textarea that will load the CKEditor.
 */
#define HTML_HOLDER									\
	"<html>"									\
	"<head><script src='" HELP_EDIT_SCRIPT_PATH "/ckeditor.js'>"			\
	"</script></head>"								\
	"<body>"									\
	"<textarea id=\"editor\" name=\"editor\">%s</textarea>"				\
	"<script>var ed = CKEDITOR.replace('editor', {fullPage:true});</script>"	\
	"</body>"									\
	"</html>"									\
	""

enum {
	PROP_0,
	PROP_FLOW
};

typedef struct _GebrHelpEditPrivate GebrHelpEditPrivate;

struct _GebrHelpEditPrivate {
	GebrGeoXmlFlow * flow;
	gchar * temp_file;
};

#define GEBR_HELP_EDIT_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE((o), GEBR_TYPE_HELP_EDIT, GebrHelpEditPrivate))

//==============================================================================
// PROTOTYPES								       =
//==============================================================================

static void gebr_help_edit_set_property	(GObject	*object,
					 guint		 prop_id,
					 const GValue	*value,
					 GParamSpec	*pspec);
static void gebr_help_edit_get_property	(GObject	*object,
					 guint		 prop_id,
					 GValue		*value,
					 GParamSpec	*pspec);
static void gebr_help_edit_destroy(GtkObject *object);

static void gebr_help_edit_commit_changes(GebrGuiHelpEdit * self);

static gchar * gebr_help_edit_get_content(GebrGuiHelpEdit * self);

static void gebr_help_edit_set_content(GebrGuiHelpEdit * self, const gchar * content);

G_DEFINE_TYPE(GebrHelpEdit, gebr_help_edit, GEBR_GUI_TYPE_HELP_EDIT);

//==============================================================================
// GOBJECT RELATED FUNCTIONS						       =
//==============================================================================

static void gebr_help_edit_class_init(GebrHelpEditClass * klass)
{
	GObjectClass *gobject_class;
	GtkObjectClass *object_class;
	GebrGuiHelpEditClass *super_class;

	gobject_class = G_OBJECT_CLASS(klass);
	object_class = GTK_OBJECT_CLASS(klass);
	super_class = GEBR_GUI_HELP_EDIT_CLASS(klass);

	gobject_class->set_property = gebr_help_edit_set_property;
	gobject_class->get_property = gebr_help_edit_get_property;
	object_class->destroy = gebr_help_edit_destroy;
	super_class->commit_changes = gebr_help_edit_commit_changes;
	super_class->get_content = gebr_help_edit_get_content;
	super_class->set_content = gebr_help_edit_set_content;

	/**
	 * GebrHelpEdit:geoxml-flow:
	 * The #GebrGeoXmlFlow associated with this help edition.
	 */
	g_object_class_install_property(gobject_class,
					PROP_FLOW,
					g_param_spec_pointer("geoxml-flow",
							     "GeoXml Flow",
							     "The GebrGeoXmlFlow associated with this help edition.",
							     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_type_class_add_private(klass, sizeof(GebrHelpEditPrivate));
}

static void gebr_help_edit_init(GebrHelpEdit * self)
{
}

static void gebr_help_edit_set_property(GObject		*object,
					guint		 prop_id,
					const GValue	*value,
					GParamSpec	*pspec)
{
	GebrHelpEditPrivate * priv;
	priv = GEBR_HELP_EDIT_GET_PRIVATE(object);

	switch (prop_id) {
	case PROP_FLOW:
		priv->flow = GEBR_GEOXML_FLOW(g_value_get_pointer(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void gebr_help_edit_get_property(GObject		*object,
					guint		 prop_id,
					GValue		*value,
					GParamSpec	*pspec)
{
	GebrHelpEditPrivate * priv;
	priv = GEBR_HELP_EDIT_GET_PRIVATE(object);

	switch (prop_id) {
	case PROP_FLOW:
		g_value_set_pointer(value, priv->flow);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void gebr_help_edit_destroy(GtkObject *object)
{
}

//==============================================================================
// PRIVATE FUNCTIONS							       =
//==============================================================================

/*
 * pre_process_html:
 * Pre process the @html string, inserting the CSS header, javascript includes,
 * etc.
 *
 * @html: The #GString to be modified.
 */
static void pre_process_html(GString * html)
{
	// Do nothing for now ;)
}

static void on_load_finished(WebKitWebView * view, WebKitWebFrame * frame)
{
	JSContextRef context;

	context = webkit_web_frame_get_global_context(frame);
}

//==============================================================================
// IMPLEMENTATION OF ABSTRACT FUNCTIONS					       =
//==============================================================================
static void gebr_help_edit_commit_changes(GebrGuiHelpEdit * self)
{
	gchar * content;
	GebrHelpEditPrivate * priv;

	priv = GEBR_HELP_EDIT_GET_PRIVATE(self);
	content = gebr_help_edit_get_content(self);
	gebr_geoxml_document_set_help(GEBR_GEOXML_DOCUMENT(priv->flow), content);
	g_free(content);
}

static gchar * gebr_help_edit_get_content(GebrGuiHelpEdit * self)
{
	// Executes the JavaScript code:
	//   return ed.getData();

	JSContextRef context;
	JSValueRef value;
	GString * str;

	context = gebr_gui_help_edit_get_js_context(self);
	value = gebr_js_evaluate(context, "ed.getData();");
	str = gebr_js_value_get_string(context, value);
	return g_string_free(str, FALSE);
}

static void gebr_help_edit_set_content(GebrGuiHelpEdit * self, const gchar * content)
{
	// Executes the JavaScript code:
	//   ed.setData(content);

	gchar * script;
	JSContextRef context;

	script = g_strdup_printf("ed.setData(%s);", content);
	context = gebr_gui_help_edit_get_js_context(self);
	gebr_js_evaluate(context, script);

	g_free(script);
}

//==============================================================================
// PUBLIC FUNCTIONS							       =
//==============================================================================
GtkWidget * gebr_help_edit_new(GebrGeoXmlFlow * flow, const gchar * content)
{
	FILE * fp;
	gchar * escaped;
	GString * help;
	GString * temp_file;
	GtkWidget * web_view;
	GebrGuiHelpEdit * self;
	GebrHelpEditPrivate * priv;

	self = g_object_new(GEBR_TYPE_HELP_EDIT,
			    "geoxml-flow", flow,
			    NULL);

	escaped = g_markup_escape_text(content, -1);
	help = g_string_new(escaped);
	web_view = gebr_gui_help_edit_get_web_view(self);
	temp_file = gebr_make_temp_filename("XXXXXX.html");
	priv = GEBR_HELP_EDIT_GET_PRIVATE(self);
	priv->temp_file = g_string_free(temp_file, FALSE);

	g_free(escaped);

	if (!help->len)
		g_string_assign(help, " ");

	pre_process_html(help);

	fp = fopen(priv->temp_file, "w");

	/* Return with a warning, if we could not create the temporary file */
	g_return_val_if_fail(fp != NULL, GTK_WIDGET(self));

	/* Return with a warning, if we could not insert content into the
	 * temporary file */
	g_return_val_if_fail(fprintf(fp, HTML_HOLDER, help->str) >= 0, GTK_WIDGET(self));

	g_signal_connect(web_view, "load-finished", G_CALLBACK(on_load_finished), NULL);

	webkit_web_view_open(WEBKIT_WEB_VIEW(web_view), priv->temp_file);

	fclose(fp);

	return GTK_WIDGET(self);
}
