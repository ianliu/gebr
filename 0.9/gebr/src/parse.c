/*   G�BR - An environment for seismic processing.
 *   Copyright (C) 2007 G�BR core team (http://gebr.sourceforge.net)
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

/*
 * File: parse.c
 * Parse pipe separated strings. Used to read/write menus's index.
 */
#include "parse.h"

#include <stdlib.h>
#include <string.h>

#define PARSE_TOKEN "|"

int
read_line (char *string, int n, FILE *fp)
{
   if (fgets (string, n, fp) == NULL) return (EXIT_SUCCESS);
   if (string[0] == 0) return (EXIT_FAILURE);
   if (string[strlen (string)-1] == '\n') string[strlen (string)-1]= 0;
   return (EXIT_FAILURE);
}

/*
 * Function: desmembra
 *
 * Quebra a linha armazenada em string em, no m�ximo, n partes e as
 * armazena em part. Retorna o n�mero de partes encontradas.
 */
int
desmembra (char *string, int n, char **part)
{
   int ii = 0;

   part[0] = strtok (string, PARSE_TOKEN);

   while ((part[ii] != NULL) && (ii < n)){
      ii++;
      part[ii] = strtok (NULL, PARSE_TOKEN);
   }

   return ii;
}

