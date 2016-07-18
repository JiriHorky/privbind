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

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include "list.h"

int parseintlist(char *str, intlist_t * list) {
  return parselist(str, list, INT_MIN, INT_MAX);
}

int parselist(char *str, intlist_t * list, int min_value, int max_value) {

  int len = strlen(str);
  char *tmp = malloc(len + 1);
  if (tmp == NULL) {
    fprintf(stderr, "privbind: Error parsing list - out of memory\n");
    return -1;
  }
  memcpy(tmp, str, len + 1);

  // count of numbers, replace comma character with zero byte
  list->count = 0;
  for (int pos = 0; pos <= len; pos++) {
    if (tmp[pos] == CHAR_COMMA || tmp[pos] == 0) {
      list->count++;
      tmp[pos] = 0;
    }
  }
  list->values = malloc(list->count * sizeof (int));
  if (list->values == NULL) {
    fprintf(stderr, "privbind: Error parsing list - out of memory\n");
    goto fail;
  }
  // convert numbers in list
  char *num_start = tmp;
  int i = 0;
  for (int pos = 0; pos <= len; pos++) {
    if (tmp[pos] == 0) {
      char *ptr;
      long int l = strtol(num_start, &ptr, 10);
      if (num_start == ptr || *ptr != 0) {
        fprintf(stderr, "privbind: Can't parse list of numbers. It is a number list?\n");
        goto fail_result;
      }
      if ( l < ((long int) min_value) || l > ((long int) max_value) ) {
        fprintf(stderr, "privbind: Can't parse list of numbers. Number %d is out of range <%d, %d>.\n", 
                l, min_value, max_value);
        goto fail_result;
      }
      list->values[i] = (int) l;
      i++;
      num_start = tmp + pos + 1;
    }
  }
  free(tmp);
  return 0;

fail_result: // fail, result is allocated already
  free(list->values);
fail: // fail, tmp is allocated already
  free(tmp);
  return -1;
}

bool is_in_list(const intlist_t * list, int value) {
  for (int i = 0; i < list->count; i++) {
    if (value == list->values[i]) {
      return true;
    }
  }
  return false;
}
