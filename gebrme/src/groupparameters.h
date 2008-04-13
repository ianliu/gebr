/*   GeBR ME - GeBR Menu Editor
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
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

#ifndef __GROUP_PARAMETERS_H
#define __GROUP_PARAMETERS_H

#include "parameters.h"
struct parameter_data;

struct group_parameters_data {
	struct parameters_data	parameters;

	struct parameter_data *	parameter;
	GtkWidget *		widget;

	/* for an exclusive group */
	GSList *		radio_group;
};

struct group_parameters_data *
group_parameters_create_ui(struct parameter_data * parameter_data, gboolean expanded);

void
group_parameters_reset_exclusive(struct group_parameters_data * data);

#endif //__GROUP_PARAMETERS_H
