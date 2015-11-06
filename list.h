/*
 * privbind - allow unpriviledged apps to bind to priviledged ports
 * Copyright (C) 2015 Lukáš karas <karas@avast.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdbool.h>

#ifndef LISTPARSER_H
#define	LISTPARSER_H

#ifdef	__cplusplus
extern "C" {
#endif

#define CHAR_COMMA ','
  
typedef struct
{
    int  count;
    int* values;
}
intlist_t;

/**
 * 
 * @param str - zero ended string contains comma separated numbers
 * @param list - 
 * @return non-zero if some error occurs
 */
int parselist(char *str, intlist_t * list);

/**
 * @param list
 * @param value
 * @return true(1) if list contains given value
 */
bool is_in_list(const intlist_t * list, int value);

#ifdef	__cplusplus
}
#endif

#endif	/* LISTPARSER_H */

