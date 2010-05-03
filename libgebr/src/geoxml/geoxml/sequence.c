/*   libgebr - GeBR Library
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

#include <string.h>

#include <gdome.h>

#include "sequence.h"
#include "xml.h"
#include "types.h"
#include "error.h"
#include "parameter.h"
#include "parameter_p.h"
#include "parameters.h"
#include "parameters_p.h"

/*
 * internal structures and funcionts
 */

struct gebr_geoxml_sequence {
	GdomeElement *element;
};

/**
 * \internal
 * Check \p sequence is a parameter
 */
static inline gboolean __gebr_geoxml_sequence_is_parameter(GebrGeoXmlSequence * sequence)
{
	return (gboolean) ! strcmp(gdome_el_nodeName((GdomeElement *) sequence, &exception)->str, "parameter");
}

/**
 * \internal
 * Check if \p sequence is really a sequence and can be modified (the case of parameter in a group)
 */
static int __gebr_geoxml_sequence_check(GebrGeoXmlSequence * sequence, gboolean check_master_instance)
{
	if (sequence == NULL)
		return GEBR_GEOXML_RETV_NULL_PTR;

	GdomeDOMString *tag;

	tag = gdome_el_tagName((GdomeElement *) sequence, &exception);
	if (check_master_instance && !strcmp(tag->str, "parameter")) {
		GebrGeoXmlParameters *parameters;

		parameters = (GebrGeoXmlParameters *) gdome_el_parentNode((GdomeElement *) sequence, &exception);
		if (__gebr_geoxml_parameters_group_check(parameters) == FALSE)
			return GEBR_GEOXML_RETV_NOT_MASTER_INSTANCE;

		return GEBR_GEOXML_RETV_SUCCESS;
	}
	/* on success, return 0 = GEBR_GEOXML_RETV_SUCCESS */
	return strcmp(tag->str, "server") &&
	    strcmp(tag->str, "value") &&
	    strcmp(tag->str, "default") &&
	    strcmp(tag->str, "option") &&
	    (!strcmp(tag->str, "parameters") &&
	     !gebr_geoxml_parameters_get_is_in_group((GebrGeoXmlParameters *) sequence)) &&
	    strcmp(tag->str, "program") &&
	    strcmp(tag->str, "category") &&
	    strcmp(tag->str, "revision") &&
	    strcmp(tag->str, "flow") && strcmp(tag->str, "path") && strcmp(tag->str, "line");
}

int
__gebr_geoxml_sequence_move_after_before(GebrGeoXmlSequence * sequence, GebrGeoXmlSequence * position,
					 int (*move_function) (GebrGeoXmlSequence * sequence,
							       GebrGeoXmlSequence * position))
{
	GebrGeoXmlParameterGroup *group;
	int ret;

	if (!__gebr_geoxml_sequence_is_parameter(sequence))
		return move_function(sequence, position);

	if ((group = gebr_geoxml_parameter_get_group(GEBR_GEOXML_PARAMETER(sequence))) != NULL) {
		GSList *sequence_refs;
		GSList *i;

		if ((ret = move_function(sequence, position)))
			return ret;

		sequence_refs = __gebr_geoxml_parameter_get_referencee_list((GdomeElement *) group,
									    __gebr_geoxml_get_attr_value((GdomeElement
													  *) sequence,
													 "id"));
		if (group == gebr_geoxml_parameter_get_group((GebrGeoXmlParameter *) position)) {
			GSList *position_refs;
			GSList *j;

			position_refs = __gebr_geoxml_parameter_get_referencee_list((GdomeElement *) group,
										    __gebr_geoxml_get_attr_value((GdomeElement *) position, "id"));
			for (i = sequence_refs, j = position_refs;
			     i != NULL && j != NULL; i = g_slist_next(i), j = g_slist_next(j))
				move_function((GebrGeoXmlSequence *) i->data, (GebrGeoXmlSequence *) j->data);

			g_slist_free(position_refs);
		} else {
			for (i = sequence_refs; i != NULL; i = g_slist_next(i))
				gdome_n_removeChild(gdome_el_parentNode((GdomeElement *) i->data, &exception),
						    (GdomeNode *) i->data, &exception);

			__gebr_geoxml_parameters_do_insert_in_group_stuff(gebr_geoxml_parameter_get_parameters
									  (GEBR_GEOXML_PARAMETER(sequence)),
									  GEBR_GEOXML_PARAMETER(sequence));
		}

		g_slist_free(sequence_refs);
	} else {
		if ((ret = move_function(sequence, position)))
			return ret;

		if (gebr_geoxml_parameter_get_group((GebrGeoXmlParameter *) position) != NULL)
			__gebr_geoxml_parameters_do_insert_in_group_stuff(gebr_geoxml_parameter_get_parameters
									  (GEBR_GEOXML_PARAMETER(sequence)),
									  GEBR_GEOXML_PARAMETER(sequence));
	}

	return ret;
}

int __gebr_geoxml_sequence_previous(GebrGeoXmlSequence ** sequence)
{
	*sequence = (GebrGeoXmlSequence *) __gebr_geoxml_previous_same_element((GdomeElement *) * sequence);

	return (*sequence != NULL) ? GEBR_GEOXML_RETV_SUCCESS : GEBR_GEOXML_RETV_INVALID_INDEX;
}

int __gebr_geoxml_sequence_next(GebrGeoXmlSequence ** sequence)
{
	*sequence = (GebrGeoXmlSequence *) __gebr_geoxml_next_same_element((GdomeElement *) * sequence);

	return (*sequence != NULL) ? GEBR_GEOXML_RETV_SUCCESS : GEBR_GEOXML_RETV_INVALID_INDEX;
}

void __gebr_geoxml_sequence_remove(GebrGeoXmlSequence * sequence)
{
	gdome_n_removeChild(gdome_el_parentNode((GdomeElement *) sequence, &exception),
			    (GdomeNode *) sequence, &exception);
}

GebrGeoXmlSequence *__gebr_geoxml_sequence_append_clone(GebrGeoXmlSequence * sequence)
{
	GebrGeoXmlSequence *clone;
	GebrGeoXmlSequence *after_last;

	clone = (GebrGeoXmlSequence *) gdome_n_cloneNode((GdomeNode *) sequence, TRUE, &exception);
	__gebr_geoxml_element_reassign_ids((GdomeElement *) clone);

	after_last = sequence;
	while (__gebr_geoxml_sequence_next(&after_last) == GEBR_GEOXML_RETV_SUCCESS);
	gdome_n_insertBefore_protected(gdome_el_parentNode((GdomeElement *) sequence, &exception),
			     (GdomeNode *) clone, (GdomeNode *) after_last, &exception);

	return clone;
}

int __gebr_geoxml_sequence_move_before(GebrGeoXmlSequence * sequence, GebrGeoXmlSequence * position)
{
	if (position == NULL) {
		GebrGeoXmlSequence *last_element, *tmp;

		last_element = tmp = sequence;
		while (!__gebr_geoxml_sequence_next(&tmp))
			last_element = tmp;
		if (last_element != sequence) {
			last_element = (GebrGeoXmlSequence *) __gebr_geoxml_next_element((GdomeElement *) last_element);
			gdome_n_insertBefore_protected(gdome_el_parentNode((GdomeElement *) sequence, &exception),
					     (GdomeNode *) sequence, (GdomeNode *) last_element, &exception);
		} else
			exception = GDOME_NOEXCEPTION_ERR;
	} else if (sequence != position)
		gdome_n_insertBefore_protected(gdome_el_parentNode((GdomeElement *) position, &exception),
				     (GdomeNode *) sequence, (GdomeNode *) position, &exception);
	else
		exception = GDOME_NOEXCEPTION_ERR;

	return exception == GDOME_NOEXCEPTION_ERR ? GEBR_GEOXML_RETV_SUCCESS : GEBR_GEOXML_RETV_DIFFERENT_SEQUENCES;
}

int __gebr_geoxml_sequence_move_after(GebrGeoXmlSequence * sequence, GebrGeoXmlSequence * position)
{
	if (position == NULL) {
		GebrGeoXmlSequence *first_element, *tmp;

		first_element = tmp = sequence;
		while (!__gebr_geoxml_sequence_previous(&tmp))
			first_element = tmp;
		if (first_element != sequence)
			gdome_n_insertBefore_protected(gdome_el_parentNode((GdomeElement *) sequence, &exception),
					     (GdomeNode *) sequence, (GdomeNode *) first_element, &exception);
		else
			exception = GDOME_NOEXCEPTION_ERR;
	} else if (sequence != position) {
		GdomeElement *next_element;

		next_element = __gebr_geoxml_next_element((GdomeElement *) position);
		gdome_n_insertBefore_protected(gdome_el_parentNode((GdomeElement *) position, &exception),
					       (GdomeNode *) sequence, (GdomeNode *) next_element, &exception);
	} else
		exception = GDOME_NOEXCEPTION_ERR;

	return exception == GDOME_NOEXCEPTION_ERR ? GEBR_GEOXML_RETV_SUCCESS : GEBR_GEOXML_RETV_DIFFERENT_SEQUENCES;
}

int __gebr_geoxml_sequence_move_up(GebrGeoXmlSequence * sequence)
{
	GebrGeoXmlSequence *previous;

	previous = sequence;
	gebr_geoxml_sequence_previous(&previous);
	if (previous == NULL)
		return GEBR_GEOXML_RETV_INVALID_INDEX;

	gdome_n_insertBefore_protected(gdome_el_parentNode((GdomeElement *) sequence, &exception),
			     (GdomeNode *) sequence, (GdomeNode *) previous, &exception);

	return GEBR_GEOXML_RETV_SUCCESS;
}

int __gebr_geoxml_sequence_move_down(GebrGeoXmlSequence * sequence)
{
	GebrGeoXmlSequence *next;

	next = sequence;
	gebr_geoxml_sequence_next(&next);
	if (next == NULL)
		return GEBR_GEOXML_RETV_INVALID_INDEX;
	next = (GebrGeoXmlSequence *) __gebr_geoxml_next_element((GdomeElement *) next);

	gdome_n_insertBefore_protected(gdome_el_parentNode((GdomeElement *) sequence, &exception),
			     (GdomeNode *) sequence, (GdomeNode *) next, &exception);

	return GEBR_GEOXML_RETV_SUCCESS;
}

/*
 * library functions.
 */

int gebr_geoxml_sequence_previous(GebrGeoXmlSequence ** sequence)
{
	int ret;
	if ((ret = __gebr_geoxml_sequence_check(*sequence, FALSE))) {
		*sequence = NULL;
		return ret;
	}

	return __gebr_geoxml_sequence_previous(sequence);
}

int gebr_geoxml_sequence_next(GebrGeoXmlSequence ** sequence)
{
	int ret;
	if ((ret = __gebr_geoxml_sequence_check(*sequence, FALSE))) {
		*sequence = NULL;
		return ret;
	}

	return __gebr_geoxml_sequence_next(sequence);
}

GebrGeoXmlSequence *gebr_geoxml_sequence_append_clone(GebrGeoXmlSequence * sequence)
{
	GebrGeoXmlSequence *clone;

	if (__gebr_geoxml_sequence_check(sequence, TRUE))
		return NULL;
	clone = __gebr_geoxml_sequence_append_clone(sequence);
	/* append reference to clone for others group instances */
	if (__gebr_geoxml_sequence_is_parameter(sequence)
	    && gebr_geoxml_parameter_get_is_in_group((GebrGeoXmlParameter *) sequence)) {
		GdomeElement *parameter_element;

		__gebr_geoxml_foreach_element(parameter_element,
					      __gebr_geoxml_parameter_get_referencee_list((GdomeElement *)
											  gebr_geoxml_parameter_get_group
											  ((GebrGeoXmlParameter *)
											   sequence),
											  __gebr_geoxml_get_attr_value((GdomeElement *) sequence, "id")))
		    __gebr_geoxml_sequence_append_clone((GebrGeoXmlSequence *) parameter_element);
	}

	return clone;
}

gint gebr_geoxml_sequence_get_index(GebrGeoXmlSequence * sequence)
{
	gint index;

	for (index = -1; sequence != NULL; __gebr_geoxml_sequence_previous(&sequence))
		index++;

	return index;
}

//TODO: improve performance
GebrGeoXmlSequence *gebr_geoxml_sequence_get_at(GebrGeoXmlSequence * sequence, gulong index)
{
	if (sequence == NULL)
		return NULL;

	gulong i;

	i = gebr_geoxml_sequence_get_index(sequence);
	if (i == index)
		return sequence;
	if (i < index)
		for (; i < index && sequence != NULL; ++i, gebr_geoxml_sequence_next(&sequence)) ;
	else
		for (; i > index && sequence != NULL; --i, gebr_geoxml_sequence_previous(&sequence)) ;

	return sequence;
}

int gebr_geoxml_sequence_remove(GebrGeoXmlSequence * sequence)
{
	int ret;

	if ((ret = __gebr_geoxml_sequence_check(sequence, TRUE)))
		return ret;
	if (__gebr_geoxml_sequence_is_parameter(sequence)
	    && gebr_geoxml_parameter_get_is_in_group((GebrGeoXmlParameter *) sequence)) {
		GdomeElement *parameter_element;

		__gebr_geoxml_foreach_element(parameter_element,
					      __gebr_geoxml_parameter_get_referencee_list((GdomeElement *)
											  gebr_geoxml_parameter_get_group
											  ((GebrGeoXmlParameter *)
											   sequence),
											  __gebr_geoxml_get_attr_value((GdomeElement *) sequence, "id")))
		    __gebr_geoxml_sequence_remove((GebrGeoXmlSequence *) parameter_element);
	}
	/* must be done after cause we need the id */
	__gebr_geoxml_sequence_remove(sequence);

	return GEBR_GEOXML_RETV_SUCCESS;
}

gboolean gebr_geoxml_sequence_is_same_sequence(GebrGeoXmlSequence * sequence, GebrGeoXmlSequence * other)
{
	if (sequence == NULL || other == NULL)
		return FALSE;
	return (gboolean) gdome_str_equal(gdome_el_nodeName((GdomeElement *) sequence, &exception),
					  gdome_el_nodeName((GdomeElement *) other, &exception));
}

int gebr_geoxml_sequence_move_into_group(GebrGeoXmlSequence * sequence, GebrGeoXmlParameterGroup * parameter_group)
{
	int ret;
	GebrGeoXmlSequence *first_instance;

	if (parameter_group == NULL)
		return GEBR_GEOXML_RETV_NULL_PTR;
	if ((ret = __gebr_geoxml_sequence_check(sequence, TRUE)))
		return ret;
	if (!__gebr_geoxml_sequence_is_parameter(sequence))
		return GEBR_GEOXML_RETV_DIFFERENT_SEQUENCES;

	if (gebr_geoxml_parameter_get_is_in_group((GebrGeoXmlParameter *) sequence)) {
		GdomeElement *parameter_element;

		__gebr_geoxml_foreach_element(parameter_element,
					      __gebr_geoxml_parameter_get_referencee_list((GdomeElement *)
											  gebr_geoxml_parameter_get_group
											  ((GebrGeoXmlParameter *)
											   sequence),
											  __gebr_geoxml_get_attr_value((GdomeElement *) sequence, "id")))
		    __gebr_geoxml_sequence_remove((GebrGeoXmlSequence *) parameter_element);
	}

	gebr_geoxml_parameter_group_get_instance(parameter_group, &first_instance, 0);
	gdome_el_insertBefore_protected((GdomeElement *) first_instance, (GdomeNode *) sequence, NULL, &exception);
	__gebr_geoxml_parameters_do_insert_in_group_stuff(GEBR_GEOXML_PARAMETERS(first_instance),
							  GEBR_GEOXML_PARAMETER(sequence));

	return ret;
}

int gebr_geoxml_sequence_move_before(GebrGeoXmlSequence * sequence, GebrGeoXmlSequence * position)
{
	int ret;

	if ((ret = __gebr_geoxml_sequence_check(sequence, TRUE)))
		return ret;
	if (position != NULL && !gebr_geoxml_sequence_is_same_sequence(sequence, position))
		return GEBR_GEOXML_RETV_DIFFERENT_SEQUENCES;
	return __gebr_geoxml_sequence_move_after_before(sequence, position, __gebr_geoxml_sequence_move_before);
}

int gebr_geoxml_sequence_move_after(GebrGeoXmlSequence * sequence, GebrGeoXmlSequence * position)
{
	int ret;

	if ((ret = __gebr_geoxml_sequence_check(sequence, TRUE)))
		return ret;
	if (position != NULL && !gebr_geoxml_sequence_is_same_sequence(sequence, position))
		return GEBR_GEOXML_RETV_DIFFERENT_SEQUENCES;
	return __gebr_geoxml_sequence_move_after_before(sequence, position, __gebr_geoxml_sequence_move_after);
}

int gebr_geoxml_sequence_move_up(GebrGeoXmlSequence * sequence)
{
	int ret;

	if ((ret = __gebr_geoxml_sequence_check(sequence, TRUE)))
		return ret;
	if ((ret = __gebr_geoxml_sequence_move_up(sequence)))
		return ret;
	if (__gebr_geoxml_sequence_is_parameter(sequence)
	    && gebr_geoxml_parameter_get_is_in_group((GebrGeoXmlParameter *) sequence)) {
		GdomeElement *parameter_element;

		__gebr_geoxml_foreach_element(parameter_element,
					      __gebr_geoxml_parameter_get_referencee_list((GdomeElement *)
											  gebr_geoxml_parameter_get_group
											  ((GebrGeoXmlParameter *)
											   sequence),
											  __gebr_geoxml_get_attr_value((GdomeElement *) sequence, "id")))
		    __gebr_geoxml_sequence_move_up((GebrGeoXmlSequence *) parameter_element);
	}
	return GEBR_GEOXML_RETV_SUCCESS;
}

int gebr_geoxml_sequence_move_down(GebrGeoXmlSequence * sequence)
{
	int ret;

	if ((ret = __gebr_geoxml_sequence_check(sequence, TRUE)))
		return ret;
	if ((ret = __gebr_geoxml_sequence_move_down(sequence)))
		return ret;
	if (__gebr_geoxml_sequence_is_parameter(sequence)
	    && gebr_geoxml_parameter_get_is_in_group((GebrGeoXmlParameter *) sequence)) {
		GdomeElement *parameter_element;

		__gebr_geoxml_foreach_element(parameter_element,
					      __gebr_geoxml_parameter_get_referencee_list((GdomeElement *)
											  gebr_geoxml_parameter_get_group
											  ((GebrGeoXmlParameter *)
											   sequence),
											  __gebr_geoxml_get_attr_value((GdomeElement *) sequence, "id")))
		    __gebr_geoxml_sequence_move_down((GebrGeoXmlSequence *) parameter_element);
	}
	return GEBR_GEOXML_RETV_SUCCESS;
}
