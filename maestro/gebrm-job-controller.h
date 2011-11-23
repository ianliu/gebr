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

#ifndef __GEBRM_JOB_CONTROLLER_H__
#define __GEBRM_JOB_CONTROLLER_H__

#include <glib-object.h>

G_BEGIN_DECLS


#define GEBRM_TYPE_JOB_CONTROLLER            (gebrm_job_controller_get_type())
#define GEBRM_JOB_CONTROLLER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GEBRM_TYPE_JOB_CONTROLLER, GebrmJobController))
#define GEBRM_JOB_CONTROLLER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  GEBRM_TYPE_JOB_CONTROLLER, GebrmJobControllerClass))
#define GEBRM_IS_JOB_CONTROLLER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GEBRM_TYPE_JOB_CONTROLLER))
#define GEBRM_IS_JOB_CONTROLLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  GEBRM_TYPE_JOB_CONTROLLER))
#define GEBRM_JOB_CONTROLLER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  GEBRM_TYPE_JOB_CONTROLLER, GebrmJobControllerClass))


typedef struct _GebrmJobController GebrmJobController;
typedef struct _GebrmJobControllerPriv GebrmJobControllerPriv;
typedef struct _GebrmJobControllerClass GebrmJobControllerClass;

struct _GebrmJobController {
	GObject parent;
	GebrmJobControllerPriv *priv;
};

struct _GebrmJobControllerClass {
	GObjectClass class_parent;
};

GType gebrm_job_controller_get_type(void) G_GNUC_CONST;

GebrmJobController *gebrm_job_controller_new(void);

/**
 * gebrm_job_controller_create:
 *
 * Appends a job into the job controller's list.
 */
//void gebrm_job_controller_add(GebrmJobController *jc,
//                              GebrmJob *job);

G_END_DECLS

#endif /* __GEBRM_JOB_CONTROLLER_H__ */
