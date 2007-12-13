/*   GêBR ME - GêBR Menu Editor
 *   Copyright (C) 2007 GêBR core team (http://gebr.sourceforge.net)
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

#include <stdarg.h>
#include <time.h>

#include <gtk/gtk.h>
#include <glib.h>

#include "support.h"
#include "gebr.h"

/*
 * Function: confirm_action_dialog
 * Show an action confirmation dialog with formated _message_
 */
gboolean
confirm_action_dialog(const gchar * message, ...)
{
	GtkWidget *	dialog;

	gchar *		string;
	va_list		argp;
	gboolean	confirmed;

	va_start(argp, message);
	string = g_strdup_vprintf(message, argp);
	va_end(argp);

	dialog = gtk_message_dialog_new(GTK_WINDOW(gebr.window),
					GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_YES_NO,
					string);
	confirmed = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES ? TRUE : FALSE;

	gtk_widget_destroy(dialog);
	g_free(string);

	return confirmed;
}

/*
 * Function: localized_date
 * Returns an string with localized date
 */
gchar *
localized_date(gchar *isodate)
{
	GTimeVal             gtime;
	static gchar         date[100];
	struct tm *          tm;

	if (g_time_val_from_iso8601(isodate, &gtime)){
		tm = localtime(&gtime.tv_sec);
		strftime (date, 100, "%c", tm);
		return date;
	}
	else return _("Unknown");
}
