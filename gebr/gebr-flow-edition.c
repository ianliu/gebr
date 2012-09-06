/*
 * gebr-flow-edition.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2012 - GêBR Core Team (www.gebrproject.com)
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

#include "gebr-flow-edition.h"
#include "ui_flow_program.c"

#include <string.h>
#include <glib/gi18n.h>
#include <libgebr/gui/gui.h>
#include <libgebr/utils.h>
#include <libgebr/gebr-iexpr.h>
#include <libgebr/gebr-expr.h>
#include <gdk/gdkkeysyms.h>

#include "gebr.h"
#include "flow.h"
#include "document.h"
#include "ui_flow_execution.h"
#include "ui_parameters.h"
#include "callbacks.h"
#include "ui_document.h"
#include "gebr-maestro-server.h"
#include "ui_flows_io.h"

/*
 * Prototypes
 */

/**
 * \internal
 * If the server was not connected, the combo box item stay insensitive
 */

