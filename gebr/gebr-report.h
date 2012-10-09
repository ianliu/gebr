/*
 * gebr-report.h
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011 - GêBR Core team (www.gebrproject.com)
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

#ifndef __GEBR_REPORT_H__
#define __GEBR_REPORT_H__

#include <glib-object.h>

typedef struct _GebrReport GebrReport;
typedef struct _GebrReportPriv GebrReportPriv;

struct _GebrReport {
	GebrReportPriv *priv;
};

GType gebr_report_get_type(void) G_GNUC_CONST;

GebrReport *gebr_report_new(void);

#endif /* end of include guard: __GEBR_REPORT_H__ */
