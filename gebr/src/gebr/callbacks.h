/*   GeBR - An environment for seismic processing.
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

#ifndef __CALLBACKS_H
#define __CALLBACKS_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

/**
 *
 */
void on_new_activate(void);
/**
 *
 */
void on_new_project_activate(void);
/**
 *
 */
void on_new_line_activate(void);
/**
 * Select copy target depending on the context
 */
void on_copy_activate(void);
/**
 * Select paste target depending on the context
 */
void on_paste_activate(void);
/** 
 * Call #gebr_quit
 */
void on_quit_activate(void);

/** 
 * Call #document_properties_setup_ui
 */
void on_document_properties_activate(void);
/** 
 * Call #document_dict_edit_setup_ui
 */
void on_document_dict_edit_activate(void);

/**
 * Call #project_delete> from #project.c
 */
void on_project_line_delete_activate(void);

/**
 * Call #project_line_import> from #ui_project_line.c
 */
void on_project_line_import_activate(void);

/**
 * Call #project_line_export> from #ui_project_line.c
 */
void on_project_line_export_activate(void);

/**
 * Call #flow_import> from #flow.c
 */
void on_flow_import_activate(void);
/**
 * Call #flow_export> from #flow.c
 */
void on_flow_export_activate(void);
/**
 * Call #flow_delete> from #flow.c
 */
void on_flow_delete_activate(void);
/** 
 * Adjust selection and show flow IO dialog
 */
void on_flow_io_activate(void);
/**
 * Call #flow_io_setup_ui> from #flow.c
 */
void on_flow_execute_activate(void);
/** 
 * Call #flow_revision_save
 */
void on_flow_revision_save_activate(void);
/** 
 * Call #flow_browse_single_selection
 */
void on_flow_revision_show_menu(void);

void on_flow_component_help_activate(void);
/**
 * Call #flow_program_remove
 */
void on_flow_component_delete_activate(void);
/**
 *
 */
void on_flow_component_properties_activate(void);
/**
 *
 */
void on_flow_component_refresh_activate(void);
/**
 * Call #flow_edition_status_changed
 */
void on_flow_component_status_activate(GtkRadioAction * action, GtkRadioAction * current);

void on_flow_component_move_top(void);

void on_flow_component_move_bottom(void);

/**
 * Call #job_control_save
 */
void on_job_control_save(void);
/**
 *
 */
void on_job_control_cancel(void);
/**
 *
 */
void on_job_control_close(void);
/**
 *
 */
void on_job_control_clear(void);
/**
 * Call #job_control_stop
 */
void on_job_control_stop(void);

/**
 *
 */
void on_configure_preferences_activate(void);
/**
 * Show servers's configuration dialog
 */
void on_configure_servers_activate(void);

/**
 *
 */
void on_help_contents_activate(void);
/**
 * Show about dialog
 */
void on_help_about_activate(void);

/**
 *
 */
void navigation_bar_update(void);

/**
 *
 */
void on_flow_browse_show_help(void);

/**
 *
 */
void on_flow_browse_edit_help(void);

/**
 */
void on_project_line_show_help(void);

void on_project_line_edit_help(void);

G_END_DECLS
#endif				//__CALLBACKS_H
