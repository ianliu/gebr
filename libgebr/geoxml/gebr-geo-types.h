/*   libgebr - GeBR Library
 *   Copyright (C) 2011 GeBR core team (http://www.gebrproject.com/)
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

#ifndef __GEBR_GEO_TYPES_H__
#define __GEBR_GEO_TYPES_H__

#include <glib.h>

G_BEGIN_DECLS

#define GEBR_GEOXML_DOCUMENT(x) ((GebrGeoXmlDocument*)(x))
#define GEBR_GEOXML_DOC(x) GEBR_GEOXML_DOCUMENT(x)
typedef struct gebr_geoxml_document GebrGeoXmlDocument;

#define GEBR_GEOXML_ENUM_OPTION(seq) ((GebrGeoXmlEnumOption*)(seq))
typedef struct gebr_geoxml_enum_option GebrGeoXmlEnumOption;

#define GEBR_GEOXML_FLOW(doc) ((GebrGeoXmlFlow*)(doc))
typedef struct gebr_geoxml_flow GebrGeoXmlFlow;

#define GEBR_GEOXML_REVISION(seq) ((GebrGeoXmlRevision*)(seq))
typedef struct gebr_geoxml_revision GebrGeoXmlRevision;

typedef struct gebr_geoxml_category GebrGeoXmlCategory;

#define GEBR_GEOXML_LINE(doc) ((GebrGeoXmlLine*)(doc))
typedef struct gebr_geoxml_line GebrGeoXmlLine;

#define GEBR_GEOXML_LINE_FLOW(seq) ((GebrGeoXmlLineFlow*)(seq))
typedef struct gebr_geoxml_line_flow GebrGeoXmlLineFlow;

typedef struct gebr_geoxml_line_path GebrGeoXmlLinePath;

#define GEBR_GEOXML_OBJECT(object) ((GebrGeoXmlObject*)(object))
typedef struct gebr_geoxml_object GebrGeoXmlObject;

#define GEBR_GEOXML_PARAMETER(super) ((GebrGeoXmlParameter*)(super))
typedef struct gebr_geoxml_parameter GebrGeoXmlParameter;

#define GEBR_GEOXML_PARAMETER_GROUP(seq) ((GebrGeoXmlParameterGroup*)(seq))
typedef struct gebr_geoxml_parameter_group GebrGeoXmlParameterGroup;

#define GEBR_GEOXML_PARAMETERS(seq) ((GebrGeoXmlParameters*)(seq))
typedef struct gebr_geoxml_parameters GebrGeoXmlParameters;

#define GEBR_GEOXML_PROGRAM_PARAMETER(obj) ((GebrGeoXmlProgramParameter*)(obj))
typedef struct gebr_geoxml_program_parameter GebrGeoXmlProgramParameter;

#define GEBR_GEOXML_PROPERTY_VALUE(sequence) ((GebrGeoXmlPropertyValue*)(sequence))
typedef struct gebr_geoxml_property_value GebrGeoXmlPropertyValue;

#define GEBR_GEOXML_PROGRAM(seq) ((GebrGeoXmlProgram*)(seq))
typedef struct gebr_geoxml_program GebrGeoXmlProgram;

#define GEBR_GEOXML_PROJECT(doc) ((GebrGeoXmlProject*)(doc))
typedef struct gebr_geoxml_project GebrGeoXmlProject;

#define GEBR_GEOXML_PROJECT_LINE(seq) ((GebrGeoXmlProjectLine*)(seq))
typedef struct gebr_geoxml_project_line GebrGeoXmlProjectLine;

typedef struct gebr_geoxml_sequence GebrGeoXmlSequence;
#define GEBR_GEOXML_SEQUENCE(seq) ((GebrGeoXmlSequence*)(seq))

#define GEBR_GEOXML_VALUE_SEQUENCE(seq) ((GebrGeoXmlValueSequence*)(seq))
typedef struct gebr_geoxml_value_sequence GebrGeoXmlValueSequence;

typedef enum {
	GEBR_GEOXML_DOCUMENT_TYPE_FLOW = 0,
	GEBR_GEOXML_DOCUMENT_TYPE_LINE,
	GEBR_GEOXML_DOCUMENT_TYPE_PROJECT,
	GEBR_GEOXML_DOCUMENT_TYPE_UNKNOWN
} GebrGeoXmlDocumentType;

typedef enum {
	GEBR_GEOXML_FLOW_ERROR_NONE = 0,
	GEBR_GEOXML_FLOW_ERROR_NO_INPUT,
	GEBR_GEOXML_FLOW_ERROR_NO_OUTPUT,
	GEBR_GEOXML_FLOW_ERROR_NO_INFILE,
	GEBR_GEOXML_FLOW_ERROR_INVALID_INFILE,
	GEBR_GEOXML_FLOW_ERROR_NO_VALID_PROGRAMS,
	GEBR_GEOXML_FLOW_ERROR_INVALID_OUTFILE,
	GEBR_GEOXML_FLOW_ERROR_INVALID_ERRORFILE,
	GEBR_GEOXML_FLOW_ERROR_LOOP_ONLY
} GebrGeoXmlFlowError;

typedef enum {
	GEBR_GEOXML_PROGRAM_ERROR_UNKNOWN_VAR,
	GEBR_GEOXML_PROGRAM_ERROR_INVAL_EXPR,
	GEBR_GEOXML_PROGRAM_ERROR_REQ_UNFILL,
} GebrGeoXmlProgramError;

#define GEBR_GEOXML_PROGRAM_ERROR (gebr_geoxml_program_error_quark())
GQuark gebr_geoxml_program_error_quark(void);

typedef enum {
	GEBR_GEOXML_PROGRAM_STATUS_UNKNOWN = 0,
	GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED,
	GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED,
	GEBR_GEOXML_PROGRAM_STATUS_DISABLED,
} GebrGeoXmlProgramStatus;

typedef enum {
	GEBR_GEOXML_PROGRAM_CONTROL_UNKNOWN = 0,
	GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY,
	GEBR_GEOXML_PROGRAM_CONTROL_FOR
} GebrGeoXmlProgramControl;

/**
 * GebrGeoXml basic object types
 */
typedef enum {
	GEBR_GEOXML_OBJECT_TYPE_UNKNOWN = 0,
	GEBR_GEOXML_OBJECT_TYPE_PROJECT,
	GEBR_GEOXML_OBJECT_TYPE_LINE,
	GEBR_GEOXML_OBJECT_TYPE_FLOW,
	GEBR_GEOXML_OBJECT_TYPE_PROGRAM,
	GEBR_GEOXML_OBJECT_TYPE_PARAMETERS,
	GEBR_GEOXML_OBJECT_TYPE_PARAMETER,
	GEBR_GEOXML_OBJECT_TYPE_ENUM_OPTION,
} GebrGeoXmlObjectType;

/**
 * GebrGeoXmlParameterType:
 * @GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN = 0: In case of error.
 * @GEBR_GEOXML_PARAMETER_TYPE_STRING: A parameter able to store a string on it.
 * @GEBR_GEOXML_PARAMETER_TYPE_INT: A parameter able to store an integer number on it.
 * @GEBR_GEOXML_PARAMETER_TYPE_FILE: A parameter able to store a file/directory path on it.
 * @GEBR_GEOXML_PARAMETER_TYPE_FLAG: A parameter able to store the state of a flag on it.
 * @GEBR_GEOXML_PARAMETER_TYPE_FLOAT: A parameter able to store a floating point number on it.
 * @GEBR_GEOXML_PARAMETER_TYPE_RANGE: A parameter able to store a number with maximum
 *     and minimum values on it.
 * @GEBR_GEOXML_PARAMETER_TYPE_ENUM: A parameter able to store a value in a list options.
 * @GEBR_GEOXML_PARAMETER_TYPE_GROUP: A sequence of parameters.
 * @GEBR_GEOXML_PARAMETER_TYPE_REFERENCE: A reference to other parameter. If the referenced
 *     parameter is a program parameter, then this parameter will only have its value as a
 *     difference.

 * #GebrGeoXmlParameterType lists the program's parameters types
 * supported by libgeoxml. They were made to create a properly
 * link between the seismic processing softwares and this library.
 *
 * See gebr_geoxml_parameter_get_is_program_parameter()
 */
typedef enum {
	GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN = 0,
	GEBR_GEOXML_PARAMETER_TYPE_STRING,
	GEBR_GEOXML_PARAMETER_TYPE_INT,
	GEBR_GEOXML_PARAMETER_TYPE_FILE,
	GEBR_GEOXML_PARAMETER_TYPE_FLAG,
	GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	GEBR_GEOXML_PARAMETER_TYPE_RANGE,
	GEBR_GEOXML_PARAMETER_TYPE_ENUM,
	GEBR_GEOXML_PARAMETER_TYPE_GROUP,
	GEBR_GEOXML_PARAMETER_TYPE_REFERENCE,
} GebrGeoXmlParameterType;

/**
 * See gebr_geoxml_document_load()
 */
typedef void (*GebrGeoXmlDiscardMenuRefCallback) (GebrGeoXmlProgram *program,
						  const gchar       *menu,
						  gint               index);

/**
 * See gebr_geoxml_program_foreach_parameter()
 */
typedef gboolean (*GebrGeoXmlCallback) (GebrGeoXmlObject *object,
					gpointer          user_data);

G_END_DECLS

#endif /* __GEBR_GEO_TYPES_H__ */
