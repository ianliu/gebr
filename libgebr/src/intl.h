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

#ifndef __INTL_H
#define __INTL_H

#include <glib.h>

#ifdef ENABLE_NLS
#	include <libintl.h>
#	undef _
#	if GLIB_CHECK_VERSION(2,18,0)
#		define _(String) g_dgettext (PACKAGE, String)
#	else
#		define _(String) dgettext (PACKAGE, String)
#	endif
#	define Q_(String) g_strip_context ((String), gettext (String))
#	ifdef gettext_noop
#		define N_(String) gettext_noop (String)
#	else
#		define N_(String) (String)
#	endif
#else
#	define textdomain(String) (String)
#	define gettext(String) (String)
#	define dgettext(Domain,Message) (Message)
#	define dcgettext(Domain,Message,Type) (Message)
#	define bindtextdomain(Domain,Directory) (Domain)
#	define _(String) (String)
#	define Q_(String) g_strip_context ((String), (String))
#	define N_(String) (String)
#endif

#endif // __INTL_H
