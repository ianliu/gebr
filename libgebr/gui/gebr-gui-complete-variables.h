/*
 * gebr_gui-gui-complete-variables.h
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

#ifndef __GEBR_GUI_COMPLETE_VARIABLES_H__
#define __GEBR_GUI_COMPLETE_VARIABLES_H__

#include <gtk/gtk.h>
#include <libgebr/geoxml/geoxml.h>

G_BEGIN_DECLS

#define GEBR_GUI_TYPE_COMPLETE_VARIABLES		(gebr_gui_complete_variables_get_type())
#define GEBR_GUI_COMPLETE_VARIABLES(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), GEBR_GUI_TYPE_COMPLETE_VARIABLES, GebrGuiCompleteVariables))
#define GEBR_GUI_IS_COMPLETE_VARIABLES(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), GEBR_GUI_TYPE_COMPLETE_VARIABLES))
#define GEBR_GUI_COMPLETE_VARIABLES_GET_INTERFACE(inst)	(G_TYPE_INSTANCE_GET_INTERFACE((inst), GEBR_GUI_TYPE_COMPLETE_VARIABLES, GebrGuiCompleteVariablesInterface))

enum {
	GEBR_GUI_COMPLETE_VARIABLES_KEYWORD = 0,
	GEBR_GUI_COMPLETE_VARIABLES_COMPLETE_TYPE,
	GEBR_GUI_COMPLETE_VARIABLES_VARIABLE_TYPE,
	GEBR_GUI_COMPLETE_VARIABLES_DOCUMENT_TYPE,
	GEBR_GUI_COMPLETE_VARIABLES_RESULT,
	GEBR_GUI_COMPLETE_VARIABLES_NCOLS,
};

typedef enum {
	GEBR_GUI_COMPLETE_VARIABLES_TYPE_VARIABLE,
	GEBR_GUI_COMPLETE_VARIABLES_TYPE_PATH,
} GebrGuiCompleteVariablesType;

typedef struct _GebrGuiCompleteVariables GebrGuiCompleteVariables;
typedef struct _GebrGuiCompleteVariablesInterface GebrGuiCompleteVariablesInterface;

struct _GebrGuiCompleteVariablesInterface {
	GTypeInterface parent;

	GtkTreeModel *(*get_filter) (GebrGuiCompleteVariables *self,
				     GebrGeoXmlParameterType type);

	GtkTreeModel *(*get_filter_full) (GebrGuiCompleteVariables *self,
					  GebrGeoXmlParameterType type,
					  GebrGeoXmlDocumentType doc_type);
};

GType gebr_gui_complete_variables_get_type(void) G_GNUC_CONST;

GtkTreeModel *gebr_gui_complete_variables_get_filter(GebrGuiCompleteVariables *self,
						     GebrGeoXmlParameterType type);

GtkTreeModel *gebr_gui_complete_variables_get_filter_full(GebrGuiCompleteVariables *self,
							  GebrGeoXmlParameterType type,
							  GebrGeoXmlDocumentType doc_type);

G_END_DECLS

#endif /* end of include guard: __GEBR_GUI_COMPLETE_VARIABLES_H__ */

