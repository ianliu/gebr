/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com)
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

#include <regex.h>
#include <glib/gstdio.h>
#include <webkit/webkit.h>
#include <libgebr/utils.h>
#include <locale.h>

#include "debr-tmpl.h"
#include "debr-help-edit-widget.h"
#include "defines.h"

/*
 * HTML_HOLDER:
 * Defines the HTML containing the textarea that will load the CKEditor.
 */
#define HTML_HOLDER									\
	"<html>"									\
	"<head>"									\
	"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />"	\
	"<script>"									\
	"var menu_edition = false;"							\
	"function onCkEditorLoadFinished() {}"						\
	"</script>"									\
	"<script src='" DEBR_HELP_EDIT_CKEDITOR "'>"					\
	"</script>"									\
	"</head>"                                                                       \
	"<body>"									\
	"<textarea id=\"editor\" name=\"editor\"></textarea>"				\
	"<script>"									\
	"editor = CKEDITOR.replace('editor', {"						\
	"	height:300,"								\
	"	width:'99%%',"								\
	"	resize_enabled:false,"							\
	"	toolbarCanCollapse:false,"						\
	"	toolbar:["								\
	"		['Source'],"							\
	"		['Bold','Italic','Underline'],"					\
	"		['Subscript','Superscript'],"					\
	"		['Undo','Redo'],"						\
	"		['JustifyLeft','JustifyCenter','JustifyRight','JustifyBlock'],"	\
	"		['NumberedList','BulletedList'],"				\
	"		['Outdent','Indent','Blockquote','Styles'],"			\
	"		['Link','Unlink'],"						\
	"		['RemoveFormat'],"						\
	"		['Find','Replace','Table']"					\
	"	]"									\
	"});"										\
	"</script>"									\
	"</body>"									\
	"</html>"									\
	""

enum {
	PROP_0,
	PROP_OBJECT
};

typedef struct _DebrHelpEditWidgetPrivate DebrHelpEditWidgetPrivate;

struct _DebrHelpEditWidgetPrivate {
	GebrGeoXmlObject * object;
	gchar * temp_file;

	// This variable is set to TRUE when the content is commited, and to FALSE when the content is set.
	gboolean is_commited;
};

#define DEBR_HELP_EDIT_WIDGET_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE((o), DEBR_TYPE_HELP_EDIT_WIDGET, DebrHelpEditWidgetPrivate))

#define CSS_LINK "<link rel=\"stylesheet\" type=\"text/css\" href=\"file://"LIBGEBR_DATA_DIR"/gebr.css\" />"

//==============================================================================
// PROTOTYPES								       =
//==============================================================================

static void debr_help_edit_widget_set_property	(GObject	*object,
						 guint		 prop_id,
						 const GValue	*value,
						 GParamSpec	*pspec);

static void debr_help_edit_widget_get_property	(GObject	*object,
						 guint		 prop_id,
						 GValue		*value,
						 GParamSpec	*pspec);

static void debr_help_edit_widget_finalize(GObject *object);

static void debr_help_edit_widget_commit_changes(GebrGuiHelpEditWidget * self);

static gchar * debr_help_edit_widget_get_content(GebrGuiHelpEditWidget * self);

static void debr_help_edit_widget_set_content(GebrGuiHelpEditWidget * self, const gchar * content);

static gboolean debr_help_edit_widget_is_content_saved(GebrGuiHelpEditWidget * self);

static const gchar *debr_help_edit_widget_get_uri (GebrGuiHelpEditWidget *self);

static void debr_help_edit_widget_preview_enter (GebrGuiHelpEditWidget *self);

static gboolean check_editor_dirty(GebrGuiHelpEditWidget * self);

static void reset_editor_dirty(GebrGuiHelpEditWidget * self);

static gchar *get_raw_content (GebrGuiHelpEditWidget *self);

static gchar *generate_help (GebrGuiHelpEditWidget *self);

static void add_parameter (GString *tbl, GebrGeoXmlParameter *par);

static gchar *generate_parameters_list (GebrGeoXmlProgram *prog);

G_DEFINE_TYPE(DebrHelpEditWidget, debr_help_edit_widget, GEBR_GUI_TYPE_HELP_EDIT_WIDGET);

//==============================================================================
// GOBJECT RELATED FUNCTIONS						       =
//==============================================================================

static void debr_help_edit_widget_class_init(DebrHelpEditWidgetClass * klass)
{
	GObjectClass *gobject_class;
	GebrGuiHelpEditWidgetClass *super_class;

	gobject_class = G_OBJECT_CLASS(klass);
	super_class = GEBR_GUI_HELP_EDIT_WIDGET_CLASS(klass);

	gobject_class->set_property = debr_help_edit_widget_set_property;
	gobject_class->get_property = debr_help_edit_widget_get_property;
	gobject_class->finalize = debr_help_edit_widget_finalize;
	super_class->commit_changes = debr_help_edit_widget_commit_changes;
	super_class->get_content = debr_help_edit_widget_get_content;
	super_class->set_content = debr_help_edit_widget_set_content;
	super_class->is_content_saved = debr_help_edit_widget_is_content_saved;
	super_class->get_uri = debr_help_edit_widget_get_uri;
	super_class->preview_enter = debr_help_edit_widget_preview_enter;

	/**
	 * DebrHelpEditWidget:geoxml-object:
	 * The #GebrGeoXmlObject associated with this help edition.
	 */
	g_object_class_install_property(gobject_class,
					PROP_OBJECT,
					g_param_spec_pointer("geoxml-object",
							     "GeoXml Object",
							     "The GebrGeoXmlObject associated with this help edition.",
							     G_PARAM_READWRITE));

	g_type_class_add_private(klass, sizeof(DebrHelpEditWidgetPrivate));
}

static void debr_help_edit_widget_init(DebrHelpEditWidget * self)
{
	DebrHelpEditWidgetPrivate * priv;

	priv = DEBR_HELP_EDIT_WIDGET_GET_PRIVATE(self);
	priv->is_commited = FALSE;
	priv->object = NULL;
	priv->temp_file = NULL;
}

static void debr_help_edit_widget_set_property(GObject		*object,
					       guint		 prop_id,
					       const GValue	*value,
					       GParamSpec	*pspec)
{
	DebrHelpEditWidgetPrivate * priv;
	priv = DEBR_HELP_EDIT_WIDGET_GET_PRIVATE(object);

	switch (prop_id) {
	case PROP_OBJECT:
		priv->object = GEBR_GEOXML_OBJECT(g_value_get_pointer(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void debr_help_edit_widget_get_property(GObject		*object,
					       guint		 prop_id,
					       GValue		*value,
					       GParamSpec	*pspec)
{
	DebrHelpEditWidgetPrivate * priv;
	priv = DEBR_HELP_EDIT_WIDGET_GET_PRIVATE(object);

	switch (prop_id) {
	case PROP_OBJECT:
		g_value_set_pointer(value, priv->object);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void debr_help_edit_widget_finalize(GObject *object)
{
	DebrHelpEditWidgetPrivate * priv;

	priv = DEBR_HELP_EDIT_WIDGET_GET_PRIVATE(object);
	if (priv->temp_file) {
		g_unlink(priv->temp_file);
		g_free(priv->temp_file);
	}

	G_OBJECT_CLASS(debr_help_edit_widget_parent_class)->finalize(object);
}

//==============================================================================
// PRIVATE FUNCTIONS							       =
//==============================================================================

/*
 * pre_process_help:
 * @help: The #GString to be modified.
 */
static void pre_process_help(GString * help)
{
	regex_t regexp;
	regmatch_t matchptr;

	if (!help->len)
		g_string_assign(help, " ");

	// Updates the CSS path
	regcomp(&regexp, "<link[^<]*gebr.css[^<]*>", REG_NEWLINE | REG_ICASE);
	if (!regexec(&regexp, help->str, 1, &matchptr, 0)) {
		gssize start = matchptr.rm_so;
		gssize length = matchptr.rm_eo - matchptr.rm_so;
		g_string_erase(help, start, length);
		g_string_insert(help, start, CSS_LINK);
	} else {
		regcomp(&regexp, "<head>", REG_NEWLINE | REG_ICASE);
		if (!regexec(&regexp, help->str, 1, &matchptr, 0)) {
			gssize start = matchptr.rm_eo;
			g_string_insert(help, start, CSS_LINK);
		}
	}
}

static void on_load_finished(WebKitWebView * view, WebKitWebFrame * frame)
{
	JSContextRef context;

	// Disconnects ourselves to prevent subsequent load-finished calls.
	g_signal_handlers_disconnect_by_func(view,
					     on_load_finished,
					     NULL);

	// 'document_clone' is a variable for keeping things clean, since 'document' will be flooded with JavaScript
	// inclusions and CKEditor stuff.
	const gchar * init_script =
		"var document_clone = document.implementation.createDocument('', '', null);"
		"document_clone.appendChild(document.documentElement.cloneNode(true));";

	context = webkit_web_frame_get_global_context(frame);
	gebr_js_evaluate(context, init_script);
	gebr_js_include(context, DEBR_HELP_EDIT_SCRIPT);
	gebr_js_include(context, DEBR_HELP_EDIT_CKEDITOR);
}

static gboolean check_editor_dirty(GebrGuiHelpEditWidget * self)
{
	JSContextRef context;
	JSValueRef value;
	context = gebr_gui_help_edit_widget_get_js_context(self);
	value = gebr_js_evaluate(context, "checkEditorDirty();");
	return gebr_js_value_get_boolean(context, value);
}

static void reset_editor_dirty(GebrGuiHelpEditWidget * self)
{
	JSContextRef context;
	context = gebr_gui_help_edit_widget_get_js_context(self);
	gebr_js_evaluate(context, "resetEditorDirty();");
}

/*
 * Fetches the content from within the editor, not the generated help.
 * See generate_help for fetching the hole help string.
 */
static gchar *get_raw_content (GebrGuiHelpEditWidget *self)
{
	JSContextRef context;
	JSValueRef value;
	GString *str;

	context = gebr_gui_help_edit_widget_get_js_context (self);
	value = gebr_js_evaluate (context, "editor.getData();");
	str = gebr_js_value_get_string (context, value);
	return g_string_free (str, FALSE);
}

/*
 * Generates the help string by composing the edited content
 * with the informations about the menu.
 */
static gchar *generate_help (GebrGuiHelpEditWidget *self)
{
	const gchar *tmp;
	gchar *escaped;
	gchar *tmpl_str;
	GError *error = NULL;
	GString *tmpl;
	gboolean is_program;
	DebrHelpEditWidgetPrivate *priv;
	GebrGeoXmlDocument *doc = NULL;
	GebrGeoXmlProgram *prog = NULL;

	if (!g_file_get_contents (DEBR_DATA_DIR "help-template.html",
				  &tmpl_str, NULL, &error))
	{
		g_warning ("Error loading template file: %s", error->message);
		g_clear_error (&error);
		return "";
	}

	tmpl = g_string_new (tmpl_str);
	priv = DEBR_HELP_EDIT_WIDGET_GET_PRIVATE (self);

	if (gebr_geoxml_object_get_type (priv->object) == GEBR_GEOXML_OBJECT_TYPE_PROGRAM) {
		is_program = TRUE;
		prog = GEBR_GEOXML_PROGRAM (priv->object);
		doc = gebr_geoxml_object_get_owner_document (priv->object);
	} else {
		is_program = FALSE;
		doc = GEBR_GEOXML_DOCUMENT (priv->object);
	}

	// Set the title!
	tmp = is_program?
		gebr_geoxml_program_get_title (prog)
		:gebr_geoxml_document_get_title (doc);
	escaped = g_markup_escape_text (tmp, -1);
	if (strlen (escaped))
		debr_tmpl_set (tmpl, "ttl", escaped);
	g_free (escaped);

	// Set the description!
	tmp = is_program?
		gebr_geoxml_program_get_description (prog)
		:gebr_geoxml_document_get_description (doc);
	escaped = g_markup_escape_text (tmp, -1);
	if (strlen (escaped))
		debr_tmpl_set (tmpl, "ttl", escaped);
	g_free (escaped);

	// Set the categories!
	GString *catstr;
	GebrGeoXmlSequence *cat;

	catstr = g_string_new ("");
	gebr_geoxml_flow_get_category (GEBR_GEOXML_FLOW (doc), &cat, 0);

	if (cat) {
		tmp = gebr_geoxml_value_sequence_get (GEBR_GEOXML_VALUE_SEQUENCE (cat));
		escaped = g_markup_escape_text (tmp, -1);
		g_string_append (catstr, escaped);
		g_free (escaped);
		gebr_geoxml_sequence_next (&cat);
	}
	while (cat) {
		tmp = gebr_geoxml_value_sequence_get (GEBR_GEOXML_VALUE_SEQUENCE (cat));
		escaped = g_markup_escape_text (tmp, -1);
		g_string_append_printf (catstr, " | %s", escaped);
		g_free (escaped);
		gebr_geoxml_sequence_next (&cat);
	}
	if (catstr->len)
		debr_tmpl_set (tmpl, "cat", catstr->str);
	g_string_free (catstr, TRUE);

	// Set the DTD!
	tmp = gebr_geoxml_document_get_version (doc);
	debr_tmpl_set (tmpl, "dtd", tmp);

	// Sets the parameters!
	if (is_program) {
		gchar *param_list;
		param_list = generate_parameters_list (prog);
		g_free (param_list);
	} else {
		debr_tmpl_set (tmpl, "par", "");
		debr_tmpl_set (tmpl, "mpr", "");
	}

	// Get the date in English format!
	GDate *date;
	gchar datestr[20];
	gchar *oldloc;

	oldloc = setlocale(LC_TIME, NULL);
	setlocale(LC_TIME, "C");
	date = g_date_new ();
	g_date_set_time_t (date, time (NULL));
	g_date_strftime (datestr, 13, "%b %d, %Y", date);
	setlocale (LC_TIME, oldloc);

	// Sets the version!
	if (is_program) {
		tmp = gebr_geoxml_program_get_version (prog);
		debr_tmpl_set (tmpl, "ver", tmp);
	} else
		debr_tmpl_set (tmpl, "ver", datestr);

	// Sets the credits!
	if (!is_program) {
		const gchar *author;
		gchar *escaped_author;
		const gchar *email;
		gchar *escaped_email;
		gchar *credit;

		author = gebr_geoxml_document_get_author (doc);
		email = gebr_geoxml_document_get_email (doc);
		escaped_author = g_markup_escape_text (author, -1);
		escaped_email = g_markup_escape_text (email, -1);

		credit = g_strdup_printf ("<p>%s: writter by %s &lt;%s&lt;</p>",
					  datestr, escaped_author, escaped_email);

		g_free (escaped_author);
		g_free (escaped_email);
	}

	return g_string_free (tmpl, FALSE);
}

/*
 * Helper function for filling the parameters.
 */
static void add_parameter (GString *tbl, GebrGeoXmlParameter *par)
{
	GebrGeoXmlProgramParameter *ppar;

	ppar = GEBR_GEOXML_PROGRAM_PARAMETER (par);

	if (gebr_geoxml_program_parameter_get_required (ppar)) {
		g_string_append_printf (tbl,
					"<li><span class='reqlabel'>%s</span><br/>",
					gebr_geoxml_parameter_get_label (par));
	} else {
		g_string_append_printf (tbl,
					"<li><span class='label'>%s</span><br/>"
					"detailed description comes here.",
					gebr_geoxml_parameter_get_label (par));
	}

	if (gebr_geoxml_parameter_get_type (par) == GEBR_GEOXML_PARAMETER_TYPE_ENUM) {
		GebrGeoXmlSequence *enum_option;

		g_string_append (tbl, "<ul>");
		gebr_geoxml_program_parameter_get_enum_option (ppar, &enum_option, 0);
		for (; enum_option != NULL; gebr_geoxml_sequence_next(&enum_option))
			g_string_append_printf (tbl, "<li>%s</li>",
						gebr_geoxml_enum_option_get_label (
								GEBR_GEOXML_ENUM_OPTION (enum_option)));
		g_string_append (tbl, "</ul>");
	}
	g_string_append (tbl, "</li>");
}

/*
 * Generates the parameters list for @prog and returns it
 */
static gchar *generate_parameters_list (GebrGeoXmlProgram *prog)
{
	GString *tbl;
	GebrGeoXmlParameterGroup *group;
	GebrGeoXmlParameters *template;
	GebrGeoXmlParameters *params;
	GebrGeoXmlParameter *param;
	GebrGeoXmlSequence *seq;
	GebrGeoXmlSequence *sub;

	tbl = g_string_new ("<ul>");
	params = gebr_geoxml_program_get_parameters (prog);
	seq = gebr_geoxml_parameters_get_first_parameter (params);

	while (seq) {
		param = GEBR_GEOXML_PARAMETER (seq);
		if (gebr_geoxml_parameter_get_is_program_parameter (param))
			add_parameter (tbl, param);
		else {
			g_string_append_printf (tbl,
						"<li class='group'>"
						"<span class='grouplabel'>%s</span>"
						"<br/>"
						"detailed description comes here.",
						gebr_geoxml_parameter_get_label (param));

			g_string_append (tbl, "<ul>");

			group = GEBR_GEOXML_PARAMETER_GROUP (param);
			template = gebr_geoxml_parameter_group_get_template (group);
			sub = gebr_geoxml_parameters_get_first_parameter (template);
			while (sub) {
				add_parameter (tbl, param);
				gebr_geoxml_sequence_next (&sub);
			}
			g_string_append (tbl, "</ul></li>");
		}
		gebr_geoxml_sequence_next (&seq);
	}
	g_string_append (tbl, "</ul>");

	return g_string_free (tbl, FALSE);
}

//==============================================================================
// IMPLEMENTATION OF ABSTRACT FUNCTIONS					       =
//==============================================================================
static void debr_help_edit_widget_commit_changes(GebrGuiHelpEditWidget * self)
{
	gchar * content;
	DebrHelpEditWidgetPrivate * priv;

	content = debr_help_edit_widget_get_content(self);
	priv = DEBR_HELP_EDIT_WIDGET_GET_PRIVATE(self);

	switch (gebr_geoxml_object_get_type(priv->object)) {
	case GEBR_GEOXML_OBJECT_TYPE_PROGRAM:
		gebr_geoxml_program_set_help(GEBR_GEOXML_PROGRAM(priv->object), content);
		break;
	case GEBR_GEOXML_OBJECT_TYPE_FLOW:
		gebr_geoxml_document_set_help(GEBR_GEOXML_DOCUMENT(priv->object), content);
		break;
	default:
		g_free(content);
		g_return_if_reached();
	}

	reset_editor_dirty(self);
	priv->is_commited = TRUE;
	g_free(content);
}

static gchar * debr_help_edit_widget_get_content(GebrGuiHelpEditWidget * self)
{
	JSContextRef context;
	JSValueRef value;
	GString * str;

	context = gebr_gui_help_edit_widget_get_js_context(self);
	value = gebr_js_evaluate(context, "getEditorContent();");
	str = gebr_js_value_get_string(context, value);
	return g_string_free(str, FALSE);
}

static void debr_help_edit_widget_set_content(GebrGuiHelpEditWidget * self, const gchar * content)
{
	FILE * fp;
	GString * help;
	GtkWidget * web_view;
	DebrHelpEditWidgetPrivate * priv;

	priv = DEBR_HELP_EDIT_WIDGET_GET_PRIVATE(self);
	priv->is_commited = FALSE;
	help = g_string_new(content);
	pre_process_help(help);

	// Save the 'help' string into the temporary file
	fp = fopen(priv->temp_file, "w");
	g_return_if_fail(fp != NULL);
	g_return_if_fail(fputs(help->str, fp) != EOF);
	fclose(fp);

	// Tells web_view to open the temporary file.
	// Note that we always connect to the 'load-finished' signal when setting the content. The callback,
	// on_load_finished, disconnects himself from the signal, so we do not have multiple load-finished events, which
	// happens when we include a javascript file.
	web_view = gebr_gui_help_edit_widget_get_web_view(self);
	g_signal_connect(web_view, "load-finished", G_CALLBACK(on_load_finished), NULL);
	webkit_web_view_open(WEBKIT_WEB_VIEW(web_view), priv->temp_file);
}

static gboolean debr_help_edit_widget_is_content_saved(GebrGuiHelpEditWidget * self)
{
	DebrHelpEditWidgetPrivate * priv;
	priv = DEBR_HELP_EDIT_WIDGET_GET_PRIVATE(self);
	return priv->is_commited && !check_editor_dirty(self);
}

static const gchar *debr_help_edit_widget_get_uri (GebrGuiHelpEditWidget *self)
{
	DebrHelpEditWidgetPrivate *priv;

	priv = DEBR_HELP_EDIT_WIDGET_GET_PRIVATE (self);
	return priv->temp_file;
}

static void debr_help_edit_widget_preview_enter (GebrGuiHelpEditWidget *self)
{
	// Build help and show it
}

//==============================================================================
// PUBLIC FUNCTIONS							       =
//==============================================================================
GebrGuiHelpEditWidget * debr_help_edit_widget_new(GebrGeoXmlObject * object,
						  const gchar * content,
						  gboolean committed)
{
	GebrGuiHelpEditWidget * self;
	DebrHelpEditWidgetPrivate * priv;
	GString * temp_file;

	self = g_object_new(DEBR_TYPE_HELP_EDIT_WIDGET,
			    "geoxml-object", object,
			    NULL);

	// Creates a temporary file which will live until this instance lives.
	// This temporary file is used to write the Help string into it so webkit can display it.
	priv = DEBR_HELP_EDIT_WIDGET_GET_PRIVATE(self);
	temp_file = gebr_make_temp_filename("XXXXXX.html");
	priv->temp_file = g_string_free(temp_file, FALSE);

	// On newer version of WebKit there is a security option we must disable
	// so WebKit can load the external JavaScript file properly.
#if WEBKIT_CHECK_VERSION(1,1,13)
	GtkWidget * web_view;
	web_view = gebr_gui_help_edit_widget_get_web_view(self);
	g_object_set(G_OBJECT(webkit_web_view_get_settings(WEBKIT_WEB_VIEW(web_view))),
		     "enable-universal-access-from-file-uris", TRUE, NULL);
#endif

	gebr_gui_help_edit_widget_set_content(self, content);
	priv->is_commited = committed;

	return self;
}

gboolean debr_help_edit_widget_is_content_empty(DebrHelpEditWidget * self)
{
	gboolean retval;
	GString * string;
	JSValueRef value;
	JSContextRef context;
	GebrGuiHelpEditWidget * super;

	super = GEBR_GUI_HELP_EDIT_WIDGET (self);
	context = gebr_gui_help_edit_widget_get_js_context (super);
	value = gebr_js_evaluate (context, "editor.getData();");
	string = gebr_js_value_get_string (context, value);

	if (string->len > 0)
		retval = FALSE;
	else
		retval = TRUE;

	g_string_free (string, TRUE);

	return retval;
}
