#!/usr/bin/python
# -*- encoding: utf8 -*-

import re
import sys
import os
import os.path
import subprocess
import select

usage = """Usage: ./update-gpl-header.py FILE [-i]
This script must be executed in a Mercurial versioned GêBR folder.

Updates the GPL notice header in file FILE. Without the -i option, the modified
file is printed on standard output, otherwise it is modified inplace.

If the file has a GPL notice, it is updated with the last year in which the
file was modified. For instance, if the file was copyrighted to 2007 but was
also modified in 2011, the script adds the range 2007-2011.

Parameters:
     -i          Modifies the file inplace

Example:
    The example below updates all header and sources files within a project.

        for file in `find . -name '*.[ch]'` ; do
            ./update-gpl-header.py $file -i
        done

    The script also accepts standard input, in which case the -i option is
    ignored. Note that the file path is still needed, as it is part of the GPL
    notice.

    The example below shows how to generate the GPL notice for a file:

        echo '' | ./update-gpl-header.py gebr-client/client.h
"""

gpl = """/*
 * %s
 * This file is part of GêBR Project
 *
 * Copyright (C) %s - GêBR Core Team (www.gebrproject.com)
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
"""

if len(sys.argv) == 1:
    sys.stderr.write(usage)
    sys.exit(1)

source_file = sys.argv[1]

if not source_file.endswith('.c') and not source_file.endswith('.h'):
    sys.stderr.write("File must end with .c or .h!\n")
    sys.exit(1)

if subprocess.call(['hg', 'root'], stdout=open('/dev/null', 'w'), stderr=subprocess.STDOUT) != 0:
    sys.stderr.write("No mercurial repository found!\n")
    sys.exit(1)

if select.select([0,], [], [], 0.0)[0]:
    has_stdin = True
else:
    has_stdin = False

if has_stdin:
    fhandler = sys.stdin
else:
    fhandler = open(source_file, 'r')

comment_start = 0
comment_end = 0
copyright_year_start = None
copyright_year_end = None
need_replace = False

STATE_BEGIN  = 0
STATE_DATE   = 1
STATE_FINISH = 2

i = 0
state = STATE_BEGIN
date_pattern = re.compile(r'copyright \(c\) (\d{4})-?(\d{4})?', re.I)
lines = []
for line in fhandler:
    lines.append(line)
    line = line.strip()
    i += 1
    if state == STATE_BEGIN:
        if line == '':
            continue
        if line.startswith('/*'):
            comment_start = i
            state = STATE_DATE
    elif state == STATE_DATE:
        if line.endswith('*/'):
            comment_end = i
            state = STATE_FINISH
        match = date_pattern.search(line)
        if match is not None:
            need_replace = True
            copyright_year_start = match.group(1)
fhandler.close()

def get_file_last_year():
    s = subprocess.check_output(['hg', 'log', '--limit', '1',
        '--template', "{date|shortdate}", source_file])
    return s[:4]

def get_copyright_notice(year1, year2 = None):
    filename = os.path.basename(source_file)
    years = year1
    if year1 != year2:
        years += "-" + year2
    cp = gpl % (filename, years)
    return cp

copyright_year_end = get_file_last_year()
if copyright_year_start is None:
    copyright_year_start = copyright_year_end

cp = get_copyright_notice(copyright_year_start, copyright_year_end)

if not has_stdin and len(sys.argv) >= 3 and sys.argv[2] == '-i':
    fhandler = open(source_file, 'w')
else:
    fhandler = sys.stdout

if need_replace:
    for line in lines[:comment_start-1]:
        fhandler.write(line)
    fhandler.write(cp)
    for line in lines[comment_end:]:
        fhandler.write(line)
else:
    fhandler.write(cp + "\n")
    for line in lines:
        fhandler.write(line)
