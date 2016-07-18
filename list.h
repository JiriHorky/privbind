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

#ifndef LIST_H
#define	LIST_H

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
 * Parse string "str" with comma separated list of numbers and fills up 
 * list structure.
 * 
 * @param str - zero ended string contains comma separated numbers
 * @param list - 
 * @return non-zero if some error occurs
 */
int parseintlist(char *str, intlist_t * list);

/**
 * Same as previos. Numbers in list have to be in given range (inclusive).
 * 
 * @param str - zero ended string contains comma separated numbers
 * @param list
 * @param min_value - minimum value (inclusive)
 * @param max_value - maximum value (inclusive)
 * @return 
 */
int parselist(char *str, intlist_t * list, int min_value, int max_value);

/**
 * @param list
 * @param value
 * @return true(1) if list contains given value
 */
bool is_in_list(const intlist_t * list, int value);

#ifdef	__cplusplus
}
#endif

#endif	/* LIST_H */

