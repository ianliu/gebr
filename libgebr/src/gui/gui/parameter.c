/*   libgebr - GÍBR Library
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

//remove round warning
#define _ISOC99_SOURCE
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "parameter.h"
#include "gtkfileentry.h"

#define DOUBLE_MAX +999999999
#define DOUBLE_MIN -999999999

/*
 * Section: Private
 * Internal stuff
 */

/*
 * Function: parameter_widget_weak_ref
 * Free struct before widget deletion
 */
static void
parameter_widget_weak_ref(struct parameter_widget * parameter_widget, GtkWidget * widget)
{
	g_free(parameter_widget);
}

/*
 * Function: parameter_widget_init
 * Create a new parameter widget based on __parameter_ and _value_widget_
 */
static struct parameter_widget *
parameter_widget_init(GeoXmlParameter * _parameter, GtkWidget * value_widget, gboolean use_default_value)
{
	struct parameter_widget *	parameter_widget;
	GtkWidget *			hbox;

	if (geoxml_program_parameter_get_is_list(GEOXML_PROGRAM_PARAMETER(_parameter)) == TRUE) {
		hbox = gtk_hbox_new(FALSE, 10);
		gtk_box_pack_start(GTK_BOX(hbox), value_widget, TRUE, TRUE, 0);
// 		GtkWidget *	label;



// 		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	} else
		hbox = value_widget;

	parameter_widget = g_malloc(sizeof(struct parameter_widget));
	*parameter_widget = (struct parameter_widget) {
		.widget = hbox,
		.parameter = _parameter,
		.value_widget = value_widget,
		.use_default_value = use_default_value
	};

	/* delete before widget destruction */
	g_object_weak_ref(G_OBJECT(parameter_widget->widget), (GWeakNotify)parameter_widget_weak_ref, parameter_widget);

	return parameter_widget;
}

/*
 * Function: parameter_widget_set_value
 * Set value considering _use_default_value_ flag.
 */
static void
parameter_widget_set_value(struct parameter_widget *parameter_widget, const gchar * value)
{
	(parameter_widget->use_default_value == FALSE)
		? parameter_widget_set_value(parameter_widget, value)
		: geoxml_program_parameter_set_default(GEOXML_PROGRAM_PARAMETER(parameter_widget->parameter), value);
}

/*
 * Function: parameter_widget_get_value
 * Get value considering _use_default_value_ flag.
 */
static const gchar *
parameter_widget_get_value(struct parameter_widget * parameter_widget)
{
	return (parameter_widget->use_default_value == FALSE)
		? geoxml_program_parameter_get_value(GEOXML_PROGRAM_PARAMETER(parameter_widget->parameter))
		: geoxml_program_parameter_get_default(GEOXML_PROGRAM_PARAMETER(parameter_widget->parameter));
}

/*
 * Function: validate_int
 * Validate an int parameter
 */
static void
validate_int(GtkEntry * entry)
{
	gchar		number[15];
	gchar *		valuestr;
	gdouble		value;

	valuestr = (gchar*)gtk_entry_get_text(entry);
	if (strlen(valuestr) == 0)
		return;

	value = g_ascii_strtod(valuestr, NULL);
	gtk_entry_set_text(entry, g_ascii_dtostr(number, 15, round(value)));
}

/*
 * Function: validate_float
 * Validate a float parameter
 */
static void
validate_float(GtkEntry * entry)
{
	gchar *		valuestr;
	gchar *		last;
	GString *	value;

	valuestr = (gchar*)gtk_entry_get_text(entry);
	if (strlen(valuestr) == 0)
		return;

	/* initialization */
	value = g_string_new(NULL);

	g_ascii_strtod(valuestr, &last);
	g_string_assign(value, valuestr);
	g_string_truncate(value, strlen(valuestr) - strlen(last));
	gtk_entry_set_text(entry, value->str);

	/* frees */
	g_string_free(value, TRUE);
}

/*
 * Function: validate_on_leaving
 * Call a validation function
 */
static gboolean
validate_on_leaving(GtkWidget * widget, GdkEventFocus * event, void (*function)(GtkEntry*))
{
	function(GTK_ENTRY(widget));

	return FALSE;
}

/*
 * Public functions
 */

/*
 * Function: parameter_widget_new_float
 * Add an input entry to a float parameter
 */
struct parameter_widget *
parameter_widget_new_float(GeoXmlParameter * parameter, gboolean use_default_value)
{
	struct parameter_widget *	parameter_widget;

	parameter_widget = parameter_widget_new_string(parameter, use_default_value);
	gtk_widget_set_size_request(parameter_widget->value_widget, 90, 30);

	g_signal_connect(parameter_widget->value_widget, "activate",
		GTK_SIGNAL_FUNC(validate_float), NULL);
	g_signal_connect(parameter_widget->value_widget, "focus-out-event",
		GTK_SIGNAL_FUNC(validate_on_leaving), &validate_float);

	return parameter_widget;
}

/*
 * Function: parameter_widget_new_int
 * Add an input entry to an integer parameter
 */
struct parameter_widget *
parameter_widget_new_int(GeoXmlParameter * parameter, gboolean use_default_value)
{
	struct parameter_widget *	parameter_widget;

	parameter_widget = parameter_widget_new_string(parameter, use_default_value);
	gtk_widget_set_size_request(parameter_widget->value_widget, 90, 30);

	g_signal_connect(parameter_widget->value_widget, "activate",
		GTK_SIGNAL_FUNC(validate_int), NULL);
	g_signal_connect(parameter_widget->value_widget, "focus-out-event",
		GTK_SIGNAL_FUNC(validate_on_leaving), &validate_int);

	return parameter_widget;
}

/*
 * Function: parameter_widget_new_string
 * Add an input entry to a string parameter
 */
struct parameter_widget *
parameter_widget_new_string(GeoXmlParameter * parameter, gboolean use_default_value)
{
	struct parameter_widget *	parameter_widget;
	GtkWidget *			entry;

	entry = gtk_entry_new();
	gtk_widget_set_size_request(entry, 140, 30);

	parameter_widget = parameter_widget_init(parameter, entry, use_default_value);
	gtk_entry_set_text(GTK_ENTRY(entry), parameter_widget_get_value(parameter_widget));

	return parameter_widget;
}

/*
 * Function: parameter_widget_new_range
 * Add an input entry to a range parameter
 */
struct parameter_widget *
parameter_widget_new_range(GeoXmlParameter * parameter, gboolean use_default_value)
{
	struct parameter_widget *	parameter_widget;
	GtkWidget *			spin;

	gchar *				min_str;
	gchar *				max_str;
	gchar *				inc_str;
	double				min, max, inc;

	geoxml_program_parameter_get_range_properties(
		GEOXML_PROGRAM_PARAMETER(parameter), &min_str, &max_str, &inc_str);
	min = !strlen(min_str) ? DOUBLE_MIN : atof(min_str);
	max = !strlen(max_str) ? DOUBLE_MAX : atof(max_str);
	inc = !strlen(inc_str) ? 1.0 : atof(inc_str);

	spin = gtk_spin_button_new_with_range(min, max, inc);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin), 3);
	gtk_widget_set_size_request(spin, 90, 30);

	parameter_widget = parameter_widget_init(parameter, spin, use_default_value);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin),
		atof(parameter_widget_get_value(parameter_widget)));

	return parameter_widget;
}

/*
 * Function: parameter_widget_new_file
 * Add an input entry and button to browse for a file or directory
 */
struct parameter_widget *
parameter_widget_new_file(GeoXmlParameter * parameter, gboolean use_default_value)
{
	struct parameter_widget *	parameter_widget;
	GtkWidget *			file_entry;

	/* file entry */
	file_entry = gtk_file_entry_new();
	gtk_widget_set_size_request(file_entry, 220, 30);

	parameter_widget = parameter_widget_init(parameter, file_entry, use_default_value);
	gtk_file_entry_set_path(GTK_FILE_ENTRY(file_entry),
		parameter_widget_get_value(parameter_widget));
	gtk_file_entry_set_choose_directory(GTK_FILE_ENTRY(file_entry),
		geoxml_program_parameter_get_file_be_directory(GEOXML_PROGRAM_PARAMETER(parameter)));

	return parameter_widget;
}

/*
 * Function: parameter_widget_new_flag
 * Add an input entry to a flag parameter
 */
struct parameter_widget *
parameter_widget_new_flag(GeoXmlParameter * parameter, gboolean use_default_value)
{
	struct parameter_widget *	parameter_widget;
	GtkWidget *			check_button;

	check_button = gtk_check_button_new_with_label(
		geoxml_program_parameter_get_label(GEOXML_PROGRAM_PARAMETER(parameter)));

	parameter_widget = parameter_widget_init(parameter, check_button, use_default_value);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), (use_default_value == FALSE)
		? geoxml_program_parameter_get_flag_status(GEOXML_PROGRAM_PARAMETER(parameter))
		: geoxml_program_parameter_get_flag_default(GEOXML_PROGRAM_PARAMETER(parameter)));

	return parameter_widget;
}

/*
 * Function: parameter_widget_submit
 * Syncronize input widget value with the parameter
 */
void
parameter_widget_submit(struct parameter_widget * parameter_widget)
{
	switch (geoxml_parameter_get_type(parameter_widget->parameter)) {
	case GEOXML_PARAMETERTYPE_FLOAT:
	case GEOXML_PARAMETERTYPE_INT:
	case GEOXML_PARAMETERTYPE_STRING:
		parameter_widget_set_value(parameter_widget,
			gtk_entry_get_text(GTK_ENTRY(parameter_widget->value_widget)));
		break;
	case GEOXML_PARAMETERTYPE_RANGE: {
		GString *	value;

		value = g_string_new(NULL);

		g_string_printf(value, "%f", gtk_spin_button_get_value(GTK_SPIN_BUTTON(parameter_widget->value_widget)));
		parameter_widget_set_value(parameter_widget, value->str);

		g_string_free(value, TRUE);
		break;
	}
	case GEOXML_PARAMETERTYPE_FILE:
		parameter_widget_set_value(parameter_widget,
			gtk_file_entry_get_path(GTK_FILE_ENTRY(parameter_widget->value_widget)));
		break;
	case GEOXML_PARAMETERTYPE_FLAG:
		geoxml_program_parameter_set_flag_state(GEOXML_PROGRAM_PARAMETER(parameter_widget->parameter),
			(parameter_widget->use_default_value == FALSE)
				? gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(parameter_widget->value_widget))
				: gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(parameter_widget->value_widget)));
		break;
	default:
		break;
	}
}

/*
 * Function: parameter_widget_update
 * Update input widget value from the parameter's value
 */
void
parameter_widget_update(struct parameter_widget * parameter_widget)
{
	const gchar *	value;

	value = parameter_widget_get_value(parameter_widget);
	switch (geoxml_parameter_get_type(parameter_widget->parameter)) {
	case GEOXML_PARAMETERTYPE_FLOAT:
	case GEOXML_PARAMETERTYPE_INT:
	case GEOXML_PARAMETERTYPE_STRING:
		gtk_entry_set_text(GTK_ENTRY(parameter_widget->value_widget), value);
		break;
	case GEOXML_PARAMETERTYPE_RANGE: {
		gchar *	endptr;
		double	number_value;

		number_value = strtod(value, &endptr);
		if (endptr != value)
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(parameter_widget->value_widget), number_value);
		break;
	} case GEOXML_PARAMETERTYPE_FILE:
		gtk_file_entry_set_path(GTK_FILE_ENTRY(parameter_widget->value_widget), value);
		break;
	case GEOXML_PARAMETERTYPE_FLAG:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(parameter_widget->value_widget),
			(parameter_widget->use_default_value == FALSE)
				? geoxml_program_parameter_get_flag_status(GEOXML_PROGRAM_PARAMETER(parameter_widget->parameter))
				: geoxml_program_parameter_get_flag_default(GEOXML_PROGRAM_PARAMETER(parameter_widget->parameter)));
		break;
	default:
		break;
	}
}
