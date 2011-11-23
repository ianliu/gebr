/*
 * gebrm-job-controller.h
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011 - GêBR Team <www.gebrproject.com>
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

#include "gebrm-job-controller.h"


G_DEFINE_TYPE(GebrmJobController, gebrm_job_controller, G_TYPE_OBJECT);


struct _GebrmJobControllerPriv {
	GHashTable *jobs;
};

static void
gebrm_job_controller_class_init(GebrmJobControllerClass *klass)
{
	g_type_class_add_private(klass, sizeof(GebrmJobControllerPriv));
}

static void
gebrm_job_controller_init(GebrmJobController *jc)
{
	jc->priv = G_TYPE_INSTANCE_GET_PRIVATE(jc,
					       GEBRM_TYPE_JOB_CONTROLLER,
					       GebrmJobControllerPriv);
}

GebrmJobController *
gebrm_job_controller_new(void)
{
	return g_object_new(GEBRM_TYPE_JOB_CONTROLLER, NULL);
}

//void
//gebrm_job_controller_add(GebrmJobController *jc, GebrmJob *job)
//{
//}
