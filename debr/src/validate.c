/*   DeBR - GeBR Designer
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>

#include <libgebr/intl.h>
#include <libgebr/date.h>
#include <libgebr/gui/utils.h>

#include "validate.h"
#include "debr.h"

/*
 * Prototypes
 */

struct validate {
	GtkWidget *widget;

	GtkWidget *text_view;

	GtkTextBuffer *text_buffer;
	GtkTreeIter iter;
	GebrGeoXmlFlow *menu;

	GHashTable *hotkey_table;
	guint error_count;
};

static void validate_free(struct validate *validate);
static gboolean validate_get_selected(GtkTreeIter * iter, gboolean warn_unselected);
static void validate_set_selected(GtkTreeIter * iter);
static void validate_clicked(void);
static void validate_do(struct validate *validate);

/*
 * Public functions.
 */

void validate_setup_ui(void)
{
	GtkWidget *hpanel;
	GtkWidget *scrolled_window;
	GtkWidget *frame;

	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	hpanel = gtk_hpaned_new();
	debr.ui_validate.widget = hpanel;

	/*
	 * Left side
	 */
	frame = gtk_frame_new("");
	gtk_paned_pack1(GTK_PANED(hpanel), frame, FALSE, FALSE);

	debr.ui_validate.list_store = gtk_list_store_new(VALIDATE_N_COLUMN,
							 GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER);
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(frame), scrolled_window);
	debr.ui_validate.tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(debr.ui_validate.list_store));
	gtk_container_add(GTK_CONTAINER(scrolled_window), debr.ui_validate.tree_view);
	gtk_widget_set_size_request(GTK_WIDGET(scrolled_window), 180, 30);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(debr.ui_validate.tree_view), FALSE);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(debr.ui_validate.tree_view)),
				    GTK_SELECTION_MULTIPLE);
	g_signal_connect(GTK_OBJECT(debr.ui_validate.tree_view), "cursor-changed", G_CALLBACK(validate_clicked), NULL);

	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(debr.ui_validate.tree_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "pixbuf", VALIDATE_ICON);
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(debr.ui_validate.tree_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", VALIDATE_FILENAME);

	/*
	 * Right side
	 */
	debr.ui_validate.text_view_vbox = gtk_vbox_new(FALSE, 0);
	gtk_paned_pack2(GTK_PANED(hpanel), debr.ui_validate.text_view_vbox, TRUE, TRUE);

	gtk_widget_show_all(debr.ui_validate.widget);

}

void validate_menu(GtkTreeIter * iter, GebrGeoXmlFlow * menu)
{
	struct validate *validate;
	GtkWidget *scrolled_window;
	GtkWidget *text_view;
	GtkTextBuffer *text_buffer;

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_box_pack_end(GTK_BOX(debr.ui_validate.text_view_vbox), scrolled_window, TRUE, TRUE, 0);
	text_buffer = gtk_text_buffer_new(NULL);
	text_view = gtk_text_view_new_with_buffer(text_buffer);
	gtk_widget_show(text_view);
	gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
	g_object_set(G_OBJECT(text_view), "editable", FALSE, "cursor-visible", FALSE, NULL);

	gtk_list_store_insert_after(debr.ui_validate.list_store, iter, NULL);
	validate = g_new(struct validate, 1);
	*validate = (struct validate) {
		.widget = scrolled_window,
		.text_view = text_view,
		.text_buffer = text_buffer,
		.iter = *iter,
		.menu = menu
	};

	{
		PangoFontDescription *font;

		font = pango_font_description_new();
		pango_font_description_set_family(font, "monospace");
		gtk_widget_modify_font(text_view, font);

		pango_font_description_free(font);
	}

	validate_do(validate);
	gtk_list_store_set(debr.ui_validate.list_store, iter,
			   VALIDATE_ICON, !validate->error_count ? debr.pixmaps.stock_apply : debr.pixmaps.stock_cancel,
			   VALIDATE_FILENAME, gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(menu)),
			   VALIDATE_POINTER, validate, -1);
	validate_set_selected(iter);
	gebr_gui_gtk_tree_view_scroll_to_iter_cell(GTK_TREE_VIEW(debr.ui_validate.tree_view), iter);
}

void validate_close(void)
{
	GtkTreeIter iter;
	struct validate *validate;

	gebr_gui_gtk_tree_view_foreach_selected(&iter, debr.ui_validate.tree_view) {
		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_validate.list_store), &iter, VALIDATE_POINTER, &validate, -1);
		validate_free(validate);
	}
}

void validate_clear(void)
{
	GtkTreeIter iter;

	gebr_gui_gtk_tree_model_foreach(iter, GTK_TREE_MODEL(debr.ui_validate.list_store)) {
		struct validate *validate;

		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_validate.list_store), &iter, VALIDATE_POINTER, &validate, -1);
		validate_free(validate);
	}
}

/*
 * Private functions.
 */

/**
 * \internal
 * Frees \p validate its iter and interface.
 */
static void validate_free(struct validate *validate)
{
	gtk_list_store_remove(debr.ui_validate.list_store, &validate->iter);
	gtk_widget_destroy(validate->widget);
	g_free(validate);
}

/**
 * \internal
 * Show selected menu report.
 */
static gboolean validate_get_selected(GtkTreeIter * iter, gboolean warn_unselected)
{
	if (gebr_gui_gtk_tree_view_get_selected(GTK_TREE_VIEW(debr.ui_validate.tree_view), iter) == FALSE) {
		if (warn_unselected)
			debr_message(GEBR_LOG_ERROR, _("No menu selected"));
		return FALSE;
	}

	return TRUE;
}

/**
 * \internal
 * Select \p iter.
 */
static void validate_set_selected(GtkTreeIter * iter)
{
	gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(debr.ui_validate.tree_view), iter);
	validate_clicked();
}

/**
 * \internal
 * Show selected menu report.
 */
static void validate_clicked(void)
{
	GtkTreeIter iter;
	struct validate *validate;

	if (!validate_get_selected(&iter, FALSE))
		return;
	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_validate.list_store), &iter, VALIDATE_POINTER, &validate, -1);

	gtk_container_forall(GTK_CONTAINER(debr.ui_validate.text_view_vbox), (GtkCallback) gtk_widget_hide, NULL);
	gtk_widget_show(validate->widget);
}

/* Prototypes */
static void validate_append_check(struct validate *validate, const gchar * value, int flags, const gchar * format, ...);
static void show_parameter(struct validate *validate, GebrGeoXmlParameter * parameter, gint ipar);
static void show_program_parameter(struct validate *validate, GebrGeoXmlProgramParameter * pp,
				   gint ipar, guint isubpar);

#define VALID		TRUE
#define INVALID		FALSE
enum VALIDATE_FLAGS {
	EMPTY = 1 << 0,
	CAPIT = 1 << 1,
	NOBLK = 1 << 2,
	MTBLK = 1 << 3,
	NOPNT = 1 << 4,
	EMAIL = 1 << 5,
	FILEN = 1 << 6,
	LABEL_HOTKEY = 1 << 7,
};

/**
 * \internal
 * Appends text to \p validate's text buffer applying \p text_tag to it.
 *
 * \param text_tag A GtkTextTag determining the style for \p format.
 * \param format Text to be inserted, in printf-like format.
 * \param argp List of arguments for \p format.
 *
 * \see validate_append_text
 */
static void
validate_append_text_valist(struct validate *validate, GtkTextTag * text_tag, const gchar * format, va_list argp)
{
	GtkTextIter iter;
	gchar *string;

	if (format == NULL)
		return;
	string = g_strdup_vprintf(format, argp);

	gtk_text_buffer_get_end_iter(validate->text_buffer, &iter);
	gtk_text_buffer_insert_with_tags(validate->text_buffer, &iter, string, -1, text_tag, NULL);

	g_free(string);
}

/**
 * \internal
 * Appends \p format to \p validate's text buffer, applying \p text_tag to it.
 * 
 * \see validate_append_text_valist
 */
static void validate_append_text_with_tag(struct validate *validate, GtkTextTag * text_tag, const gchar * format, ...)
{
	va_list argp;

	va_start(argp, format);
	validate_append_text_valist(validate, text_tag, format, argp);
	va_end(argp);
}

/**
 * \internal
 * Appends \p text into validate log with pair of style/values, as seen in #GtkTextTag properties.
 *
 * \see GtkTextTag
 */
static void
validate_append_text_with_property_list(struct validate *validate, const gchar * text,
					const gchar * first_property_name, ...)
{
	GtkTextTag *text_tag;
	va_list argp;

	text_tag = gtk_text_tag_new(NULL);
	gtk_text_tag_table_add(gtk_text_buffer_get_tag_table(validate->text_buffer), text_tag);
	va_start(argp, first_property_name);
	g_object_set_valist(G_OBJECT(text_tag), first_property_name, argp);
	va_end(argp);

	validate_append_text_with_tag(validate, text_tag, text);
}

/**
 * \internal
 * Appends emphasized text in validate log, with printf-like \p format.
 *
 * \see validate_append_text
 */
static void validate_append_text_emph(struct validate *validate, const gchar * format, ...)
{
	gchar *string;
	va_list argp;

	va_start(argp, format);
	string = g_strdup_vprintf(format, argp);
	validate_append_text_with_property_list(validate, string, "weight", PANGO_WEIGHT_BOLD, NULL);
	va_end(argp);
	g_free(string);
}

/**
 * \internal
 * Appends text in validate log, with printf-like \p format.
 */
static void validate_append_text(struct validate *validate, const gchar * format, ...)
{
	gchar *string;
	va_list argp;

	va_start(argp, format);
	string = g_strdup_vprintf(format, argp);
	validate_append_text_with_tag(validate, NULL, string);
	va_end(argp);
	g_free(string);
}

/**
 * \internal
 * Appends \p format into \p validate buffer, indicating error (by setting the text color to red).
 */
static void validate_append_text_error(struct validate *validate, const gchar * format, ...)
{
	gchar *string;
	va_list argp;

	va_start(argp, format);
	string = g_strdup_vprintf(format, argp);
	validate_append_text_with_property_list(validate, string, "foreground", "#ff0000", NULL);
	va_end(argp);
	g_free(string);
}

/**
 * \internal
 * Inserts \p item into validate log using #validate_append_text_emph.
 */
static void validate_append_item(struct validate *validate, const gchar * item)
{
	validate_append_text_emph(validate, item);
}

/**
 * \internal
 * Appends \p item into \p validate log and checks \p value againt \p flags.
 *
 * \see validate_append_check validate_append_item
 */
static void
validate_append_item_with_check(struct validate *validate, const gchar * item, const gchar * value, int flags)
{
	validate_append_item(validate, item);
	validate_append_check(validate, value, flags, "\n");
}

/**
 * \internal
 * Validate menu at \p validate.
 */
static void validate_do(struct validate *validate)
{
	gboolean all = TRUE;
	gboolean filename = FALSE;
	gboolean title = FALSE;
	gboolean desc = FALSE;
	gboolean author = FALSE;
	gboolean dates = FALSE;
	gboolean category = FALSE;
	gboolean mhelp = FALSE;
	gboolean progs = FALSE;
	gboolean params = FALSE;

	GebrGeoXmlSequence *seq;
	gint i;

	validate->error_count = 0;
	if (filename || all)
		validate_append_item_with_check(validate, _("Filename:      "),
						gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(validate->menu)),
						NOBLK | MTBLK | FILEN);
	if (title || all)
		validate_append_item_with_check(validate, _("Title:         "),
						gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(validate->menu)),
						EMPTY | NOBLK | NOPNT | MTBLK);
	if (desc || all)
		validate_append_item_with_check(validate, _("Description:   "),
						gebr_geoxml_document_get_description(GEBR_GEOXML_DOCUMENT
										     (validate->menu)),
						EMPTY | CAPIT | NOBLK | MTBLK | NOPNT);
	if (author || all) {
		validate_append_item(validate, _("Author:        "));
		validate_append_check(validate, gebr_geoxml_document_get_author(GEBR_GEOXML_DOCUMENT(validate->menu)),
				      EMPTY | CAPIT | NOBLK | MTBLK | NOPNT, " <");
		validate_append_check(validate, gebr_geoxml_document_get_email(GEBR_GEOXML_DOCUMENT(validate->menu)),
				      EMAIL, ">");
		validate_append_text(validate, "\n");
	}
	if (dates || all) {
		validate_append_item_with_check(validate, _("Created:       "),
						gebr_localized_date(gebr_geoxml_document_get_date_created
								    (GEBR_GEOXML_DOCUMENT(validate->menu))), EMPTY);
		validate_append_item_with_check(validate, _("Modified:      "),
						gebr_localized_date(gebr_geoxml_document_get_date_modified
								    (GEBR_GEOXML_DOCUMENT(validate->menu))), EMPTY);
	}
	if (mhelp || all) {
		validate_append_item(validate, _("Help:          "));
		if (strlen(gebr_geoxml_document_get_help(GEBR_GEOXML_DOCUMENT(validate->menu))) >= 1)
			validate_append_text(validate, _("Defined"));
		else
			validate_append_check(validate, "", EMPTY, "");
		validate_append_text(validate, "\n");
	}
	if (category || all) {
		gebr_geoxml_flow_get_category(validate->menu, &seq, 0);
		if (seq == NULL)
			validate_append_item_with_check(validate, _("Category:      "), "", EMPTY);
		else
			for (; seq != NULL; gebr_geoxml_sequence_next(&seq))
				validate_append_item_with_check(validate, _("Category:      "),
								gebr_geoxml_value_sequence_get
								(GEBR_GEOXML_VALUE_SEQUENCE(seq)),
								EMPTY | CAPIT | NOBLK | MTBLK | NOPNT);
	}

	if (!progs && !all && !params)
		goto out;

	validate_append_text_emph(validate, _("Menu with:     "));
	validate_append_text(validate, _("%ld program(s)\n"), gebr_geoxml_flow_get_programs_number(validate->menu));
	gebr_geoxml_flow_get_program(validate->menu, &seq, 0);
	for (i = 0; seq != NULL; i++, gebr_geoxml_sequence_next(&seq)) {
		GebrGeoXmlProgram *prog;
		GebrGeoXmlParameter *parameter;
		gint j = 0;

		prog = GEBR_GEOXML_PROGRAM(seq);

		validate_append_text_emph(validate, _("\n>>Program:     "));
		validate_append_text(validate, "%d\n", i + 1);
		validate_append_item_with_check(validate, _("  Title:       "),
						gebr_geoxml_program_get_title(prog), EMPTY | NOBLK | MTBLK);
		validate_append_item_with_check(validate, _("  Description: "),
						gebr_geoxml_program_get_description(prog),
						EMPTY | CAPIT | NOBLK | MTBLK | NOPNT);

		validate_append_text_emph(validate, _("  In/out/err:  "));
		validate_append_text(validate, "%s/%s/%s\n",
				     gebr_geoxml_program_get_stdin(prog) ? _("Read") : _("Ignore"),
				     gebr_geoxml_program_get_stdout(prog) ? _("Write") : _("Ignore"),
				     gebr_geoxml_program_get_stdin(prog) ? _("Append") : _("Ignore"));

		validate_append_item_with_check(validate, _("  Binary:      "),
						gebr_geoxml_program_get_binary(prog), EMPTY);
		validate_append_item_with_check(validate, _("  URL:         "),
						gebr_geoxml_program_get_url(prog), EMPTY);
		validate_append_item(validate, _("  Help:        "));
		if (strlen(gebr_geoxml_program_get_help(prog)) >= 1)
			validate_append_text(validate, _("Defined"));
		else
			validate_append_check(validate, "", EMPTY, "");
		validate_append_text(validate, "\n");

		if (params || all) {
			GHashTable *hotkey_table;

			validate->hotkey_table = hotkey_table = g_hash_table_new_full((GHashFunc)g_str_hash, (GEqualFunc)g_str_equal, (GDestroyNotify)g_free, NULL);

			validate_append_text_emph(validate, _("  >>Parameters:\n"));

			parameter = GEBR_GEOXML_PARAMETER(gebr_geoxml_parameters_get_first_parameter
							  (gebr_geoxml_program_get_parameters(prog)));

			while (parameter != NULL) {
				show_parameter(validate, parameter, ++j);
				gebr_geoxml_sequence_next((GebrGeoXmlSequence **) & parameter);
			}
			g_hash_table_unref(hotkey_table);
		}
	}

out:	validate_append_text_emph(validate, _("%d potential error(s)"), validate->error_count);
}

/**
 * \internal
 * VALID if str is not empty.
 */
static gboolean check_is_not_empty(const gchar * str)
{
	return (strlen(str) ? VALID : INVALID);
}

/**
 * \internal
 * VALID if str does not start with lower case letter.
 */
static gboolean check_no_lower_case(const gchar * sentence)
{
	if (!check_is_not_empty(sentence))
		return VALID;
	if (g_ascii_islower(sentence[0]))
		return INVALID;

	return VALID;
}

/**
 * \internal
 * VALID if str has not consecutive blanks.
 */
static gboolean check_no_multiple_blanks(const gchar * str)
{
	regex_t pattern;
	regcomp(&pattern, "   *", REG_NOSUB);
	return (regexec(&pattern, str, 0, 0, 0) ? VALID : INVALID);
}

/**
 * \internal
 * VALID if str does not start or end with blanks/tabs.
 */
static gboolean check_no_blanks_at_boundaries(const gchar * str)
{
	int n = strlen(str);

	if (n == 0)
		return VALID;
	if (str[0] == ' ' || str[0] == '\t' || str[n - 1] == ' ' || str[n - 1] == '\t')
		return INVALID;

	return VALID;
}

/**
 * \internal
 * VALID if str does not end if a punctuation mark.
 */
static gboolean check_no_punctuation_at_end(const gchar * str)
{
	int n = strlen(str);

	if (n == 0)
		return VALID;
	if (str[n - 1] != ')' && g_ascii_ispunct(str[n - 1]))
		return INVALID;

	return VALID;
}

/**
 * \internal
 * VALID if str has not path and ends with .mnu
 */
static gboolean check_menu_filename(const gchar * str)
{
	gchar *base;

	base = g_path_get_basename(str);
	if (strcmp(base, str)) {
		g_free(base);
		return INVALID;
	}
	g_free(base);

	if (!g_str_has_suffix(str, ".mnu"))
		return INVALID;

	return VALID;
}

/**
 * \internal
 * VALID if \p str is of kind <code>xxx@yyy</code>, with xxx composed by letter, digits, underscores, dots and dashes,
 * and yyy composed by at least one dot, letter digits and dashes.
 *
 * \see http://en.wikipedia.org/wiki/E-mail_address
 */
static gboolean check_is_email(const gchar * str)
{
	regex_t pattern;
	regcomp(&pattern, "^[a-z0-9_.-][a-z0-9_.-]*@[a-z0-9.-]*\\.[a-z0-9-][a-z0-9-]*$", REG_NOSUB | REG_ICASE);
	return (!regexec(&pattern, str, 0, 0, 0) ? VALID : INVALID);
}

/**
 * \internal
 * VALID if \p value passed through all selected tests
 */
static void validate_append_check(struct validate *validate, const gchar * value, int flags, const gchar * format, ...)
{
	gboolean result = VALID;
	va_list argp;

	if (flags & EMPTY)
		result = result && check_is_not_empty(value);
	if (flags & CAPIT)
		result = result && check_no_lower_case(value);
	if (flags & NOBLK)
		result = result && check_no_blanks_at_boundaries(value);
	if (flags & MTBLK)
		result = result && check_no_multiple_blanks(value);
	if (flags & NOPNT)
		result = result && check_no_punctuation_at_end(value);
	if (flags & EMAIL)
		result = result && check_is_email(value);
	if (flags & FILEN)
		result = result && check_menu_filename(value);

	if (result)
		validate_append_text(validate, value);
	else {
		if (check_is_not_empty(value))
			validate_append_text_error(validate, "%s", value);
		else
			validate_append_text_error(validate, _("UNSET"));
		validate->error_count++;
	}

	if (flags & LABEL_HOTKEY) {
		gchar * underscore;

		underscore = (gchar*)value;
		do {
			gint len;

			underscore = strchr(underscore, '_');
			if (!underscore)
				break;
			len = strlen(underscore);
			if (len == 1) //_ in the end of string	
				underscore = NULL;
			else if (len > 1 && *(underscore+1) == '_') {
				if (len == 2) //__ in the end of string	
					underscore = NULL;
				else
					underscore += 2;
			} else
				break;
		} while (underscore); 
		if (underscore) {
			GString *label_ext;
			gint length;
			gchar * uppercase;
			gchar hotkey[6];

			label_ext = g_string_new("");
			length = g_unichar_to_utf8(g_utf8_get_char(underscore + 1), hotkey);
			uppercase = g_utf8_strup(hotkey, length);
			if (g_hash_table_lookup(validate->hotkey_table, uppercase)) {
				g_string_printf(label_ext, " (Alt+%s%s)", uppercase, _(", already used above"));
				validate_append_text_error(validate, label_ext->str);
				++validate->error_count;
				result = INVALID;
			} else
				validate_append_text(validate, " (Alt+%s)", uppercase);
			g_hash_table_insert(validate->hotkey_table, uppercase, GINT_TO_POINTER(1));

			g_string_free(label_ext, TRUE);
		}
	}

	va_start(argp, format);
	validate_append_text_valist(validate, NULL, format, argp);
	va_end(argp);
}

/**
 * \internal
 */
static void show_program_parameter(struct validate *validate, GebrGeoXmlProgramParameter * pp,
				   gint ipar, guint isubpar)
{
	GString *default_value;
	const gchar * label;

	if (isubpar)
		validate_append_text(validate, "       %2d.%02d: ", ipar, isubpar);
	else
		validate_append_text(validate, "    %2d: ", ipar);

	label = gebr_geoxml_parameter_get_label(GEBR_GEOXML_PARAMETER(pp));
	validate_append_check(validate, label, EMPTY | CAPIT | NOBLK | MTBLK | NOPNT | LABEL_HOTKEY, "\n");

	validate_append_text(validate, "        ");
	if (isubpar)
		validate_append_text(validate, "      ");

	validate_append_text(validate, "[");
	validate_append_text(validate, gebr_geoxml_parameter_get_type_name(GEBR_GEOXML_PARAMETER(pp)));
	if (gebr_geoxml_program_parameter_get_is_list(GEBR_GEOXML_PROGRAM_PARAMETER(pp)))
		validate_append_text(validate, "(s)");
	validate_append_text(validate, "] ");

	validate_append_text(validate, "'");
	validate_append_check(validate, gebr_geoxml_program_parameter_get_keyword(pp), EMPTY, "'");

	default_value = gebr_geoxml_program_parameter_get_string_value(pp, TRUE);
	if (default_value->len)
		validate_append_text(validate, " [%s]", default_value->str);
	g_string_free(default_value, TRUE);

	if (gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(pp)) == GEBR_GEOXML_PARAMETER_TYPE_INT ||
	    gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(pp)) == GEBR_GEOXML_PARAMETER_TYPE_FLOAT ||
	    gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(pp)) == GEBR_GEOXML_PARAMETER_TYPE_RANGE) {

		gchar *min_str;
		gchar *max_str;

		gebr_geoxml_program_parameter_get_number_min_max(pp, &min_str, &max_str);
		validate_append_text(validate, " in [%s,%s]", (strlen(min_str) <= 0 ? "*" : min_str),
				     (strlen(max_str) <= 0 ? "*" : max_str));
	}

	if (gebr_geoxml_program_parameter_get_is_list(GEBR_GEOXML_PROGRAM_PARAMETER(pp))) {
		if (strlen(gebr_geoxml_program_parameter_get_list_separator(GEBR_GEOXML_PROGRAM_PARAMETER(pp))))
			validate_append_text(validate, _(" (entries separeted by '%s')"),
					     gebr_geoxml_program_parameter_get_list_separator
					     (GEBR_GEOXML_PROGRAM_PARAMETER(pp)));
		else {
			validate_append_text_error(validate, _(" (missing entries' separator)"));
			validate->error_count++;
		}
	}

	if (gebr_geoxml_program_parameter_get_required(pp))
		validate_append_text(validate, _("  REQUIRED "));

	/* enum details */
	if (gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(pp)) == GEBR_GEOXML_PARAMETER_TYPE_ENUM) {
		GebrGeoXmlSequence *enum_option;

		gebr_geoxml_program_parameter_get_enum_option(pp, &enum_option, 0);

		if (enum_option == NULL) {
			validate_append_text_error(validate, _("\n        missing options"));
			validate->error_count++;
		}

		for (; enum_option != NULL; gebr_geoxml_sequence_next(&enum_option)) {
			validate_append_text(validate, "\n");
			if (isubpar)
				validate_append_text(validate, "      ");

			validate_append_text(validate, "        %s (%s)",
					     gebr_geoxml_enum_option_get_label(GEBR_GEOXML_ENUM_OPTION(enum_option)),
					     gebr_geoxml_enum_option_get_value(GEBR_GEOXML_ENUM_OPTION(enum_option)));
		}
	}

	validate_append_text(validate, "\n\n");
}

/**
 * \internal
 */
static void show_parameter(struct validate *validate, GebrGeoXmlParameter * parameter, gint ipar)
{
	if (gebr_geoxml_parameter_get_is_program_parameter(parameter))
		show_program_parameter(validate, GEBR_GEOXML_PROGRAM_PARAMETER(parameter), ipar, 0);
	else {
		GebrGeoXmlSequence *subpar;
		GebrGeoXmlSequence *instance;

		gint subipar = 0;

		validate_append_text(validate, "    %2d: ", ipar);
		validate_append_check(validate, gebr_geoxml_parameter_get_label(parameter),
				      EMPTY | CAPIT | NOBLK | MTBLK | NOPNT | LABEL_HOTKEY, NULL);

		if (gebr_geoxml_parameter_group_get_is_instanciable(GEBR_GEOXML_PARAMETER_GROUP(parameter)))
			validate_append_text(validate, _("   [Instanciable]\n"));
		else
			validate_append_text(validate, "\n");

		gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
		subpar = gebr_geoxml_parameters_get_first_parameter(GEBR_GEOXML_PARAMETERS(instance));
		while (subpar != NULL) {
			show_program_parameter(validate, GEBR_GEOXML_PROGRAM_PARAMETER(subpar), ipar, ++subipar);
			gebr_geoxml_sequence_next(&subpar);
		}
	}
}

