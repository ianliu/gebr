/*
 * gebr_gui-complete-variables.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2012 - GêBR Core Team (www.gebr_guiproject.com)
 *
 * GêBR Project is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GêBR Project is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GêBR Project. If not, see <http://www.gnu.org/licenses/>.
 */

#include "gebr-gui-complete-variables.h"

static void
gebr_gui_complete_variables_default_init(GebrGuiCompleteVariablesInterface *klass)
{
}

GType
gebr_gui_complete_variables_get_type (void)
{
	static volatile gsize g_define_type_id__volatile = 0;
	if (g_once_init_enter (&g_define_type_id__volatile)) {
		GType g_define_type_id =
			g_type_register_static_simple(G_TYPE_INTERFACE,
						      g_intern_static_string ("GebrGuiCompleteVariables"),
						      sizeof (GebrGuiCompleteVariablesInterface),
						      (GClassInitFunc) gebr_gui_complete_variables_default_init,
						      0,
						      (GInstanceInitFunc) NULL,
						      (GTypeFlags) 0);
		g_type_interface_add_prerequisite (g_define_type_id, G_TYPE_OBJECT);
		g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
	}
	return g_define_type_id__volatile;
}

GtkTreeModel *
gebr_gui_complete_variables_get_filter(GebrGuiCompleteVariables *self,
				       GebrGeoXmlParameterType type)
{
	return GEBR_GUI_COMPLETE_VARIABLES_GET_INTERFACE(self)->get_filter(self, type);
}

GtkTreeModel *
gebr_gui_complete_variables_get_filter_full(GebrGuiCompleteVariables *self,
					    GebrGeoXmlParameterType type,
					    GebrGeoXmlDocumentType doc_type)
{
	return GEBR_GUI_COMPLETE_VARIABLES_GET_INTERFACE(self)->get_filter_full(self, type, doc_type);
}
