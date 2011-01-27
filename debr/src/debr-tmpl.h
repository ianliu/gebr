/*   DeBR - GeBR Designer
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

#ifndef __DEBR_TMPL_H__
#define __DEBR_TMPL_H__

#include <glib.h>

/**
 * debr_tmpl_get:
 * @tmpl: the string buffer containing the template
 * @tag: the name of the tag to be fetched
 *
 * Searchs for the text between <emphasis>&gt;-- begin @tag --&lt;</emphasis> and <emphasis>&gt;-- end @tag
 * --&lt;</emphasis> and returns a copy of it. If @tag is not found, then %NULL is returned.
 *
 * Returns: a newly allocated c-string or %NULL if @tag was not found.
 */
gchar *debr_tmpl_get (GString *tmpl, const gchar *tag);

/**
 * debr_tmpl_set:
 * @tmpl: the string buffer containing the template
 * @tag: the name of the tag to be set
 * @value: the value that will replace the tag
 *
 * Replaces the content between <emphasis>&gt;-- begin @tag --&lt;</emphasis> and <emphasis>&gt;-- end @tag
 * --&lt;</emphasis>. If @tag was not found, then %FALSE is returned and @tmpl is left unchanged.
 *
 * Returns: %TRUE if @tag was found, %FALSE otherwise.
 */
gboolean debr_tmpl_set (GString *tmpl, const gchar *tag, const gchar *value);

/**
 * debr_tmpl_append:
 * @tmpl: the string buffer containing the template
 * @tag: the name of the tag to be set
 * @value: the value that will be appended to the end of the tag
 *
 * Inserts @value at the end of @tag.
 *
 * Returns: %TRUE if @tag was found, %FALSE otherwise.
 */
gboolean debr_tmpl_append (GString *tmpl, const gchar *tag, const gchar *value);

#endif /* __DEBR_TMPL_H__ */
