/* stringvec.c -- do vector of strings properly

   Copyright 2009 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "config.h"
#include <stdio.h>
#include "incl_unix.h"
#include "stringvec.h"

static  char    Filename[] = __FILE__;

void  stringvec_init(struct stringvec *sv)
{
        sv->memb_cnt = 0;
        sv->memb_max = STRINGVEC_INIT;
        sv->memb_list = (char **) malloc(STRINGVEC_INIT * sizeof(char *));
        if  (!sv->memb_list)
                ABORT_NOMEM;
}

void  stringvec_insert_unique(struct stringvec *sv, const char *newitem)
{
        int  first = 0, last = sv->memb_cnt;

        /* This is binary search and insert */

        while  (first < last)  {
                int     mid = (first + last) / 2;
                int     cmp = strcmp(sv->memb_list[mid], newitem);
                if  (cmp == 0)
                        return;
                if  (cmp < 0)
                        first = mid + 1;
                else
                        last = mid;
        }

        /* Ready to insert at "first", move rest up */

        if  (sv->memb_cnt >= sv->memb_max)  {
                sv->memb_max += STRINGVEC_INC;
                sv->memb_list = realloc(sv->memb_list, (unsigned) (sv->memb_max * sizeof(char *)));
                if  (!sv->memb_list)
                        ABORT_NOMEM;
        }
        for  (last = sv->memb_cnt;  last > first;  last--)
                sv->memb_list[last] = sv->memb_list[last-1];

        sv->memb_list[first] = stracpy(newitem);
        sv->memb_cnt++;
}

static void  test_realloc(struct stringvec *sv)
{
        if  (sv->memb_cnt >= sv->memb_max)  {
                sv->memb_max += STRINGVEC_INC;
                sv->memb_list = realloc(sv->memb_list, (unsigned) (sv->memb_max * sizeof(char *)));
                if  (!sv->memb_list)
                        ABORT_NOMEM;
        }
}

void  stringvec_append(struct stringvec *sv, const char *newitem)
{
        test_realloc(sv);
        sv->memb_list[sv->memb_cnt] = stracpy(newitem);
        sv->memb_cnt++;
}

/* Split string with separator sep into a stringvec. */

void    stringvec_split(struct stringvec *sv, const char *str, const char sep)
{
        const   char    *start = str, *nxt = strchr(str, sep);

        stringvec_init(sv);

        while  (start)  {
                test_realloc(sv);
                if  (nxt)  {
                        unsigned  lng = nxt - start;
                        char    *piece = malloc(lng+1);
                        if  (!piece)
                                ABORT_NOMEM;
                        strncpy(piece, start, lng);
                        piece[lng] = '\0';
                        sv->memb_list[sv->memb_cnt] = piece;
                        sv->memb_cnt++;
                        start = nxt + 1;
                        nxt = strchr(start, sep);
                }
                else  {
                        stringvec_append(sv, start);
                        start = nxt;
                }
        }
}

/* As above but sort the list */

void    stringvec_split_sorted(struct stringvec *sv, const char *str, const char sep)
{
        char    *work = stracpy(str);
        char    *start = work, *nxt = strchr(work, sep);

        stringvec_init(sv);
        while  (start)  {
                if  (nxt)  {
                        *nxt = '\0';
                        stringvec_insert_unique(sv, start);
                        start = nxt + 1;
                        nxt = strchr(start, sep);
                }
                else  {
                        stringvec_insert_unique(sv, start);
                        start = nxt;
                }
        }
        free(work);
}

char    *stringvec_join(struct stringvec *sv, const char sep)
{
        char    *result, *rp;
        unsigned  totlng = 1;
        int     cnt;

        /* Get total length */

        for  (cnt = 0;  cnt < sv->memb_cnt;  cnt++)
                totlng += strlen(sv->memb_list[cnt]) + 1;

        if  (!(result = malloc(totlng)))
                ABORT_NOMEM;

        rp = result;
        for  (cnt = 0;  cnt < sv->memb_cnt;  cnt++)  {
                strcpy(rp, sv->memb_list[cnt]);
                rp += strlen(sv->memb_list[cnt]);
                *rp++ = sep;
        }
        *rp = '\0';
        return  result;
}

void  stringvec_insert(struct stringvec *sv, const int which, const char *newitem)
{
        int     lim = which, cnt;
        if  (lim > sv->memb_cnt  ||  lim < 0)
                lim = sv->memb_cnt;
        test_realloc(sv);
        for  (cnt = sv->memb_cnt;  cnt > lim;  cnt--)
                sv->memb_list[cnt] = sv->memb_list[cnt-1];
        sv->memb_list[lim] = stracpy(newitem);
        sv->memb_cnt++;
}

void  stringvec_delete(struct stringvec *sv, const unsigned which)
{
        if  (which < (unsigned) sv->memb_cnt)  {
                unsigned  cnt;
                free(sv->memb_list[which]);
                for  (cnt = which+1;  cnt < (unsigned) sv->memb_cnt;  cnt++)
                        sv->memb_list[cnt-1] = sv->memb_list[cnt];
                sv->memb_cnt--;
        }
}

void  stringvec_replace(struct stringvec *sv, const unsigned which, const char *newitem)
{
        if  (which < (unsigned) sv->memb_cnt)  {
                free(sv->memb_list[which]);
                sv->memb_list[which] = stracpy(newitem);
        }
}

void  stringvec_free(struct stringvec *sv)
{
        int     cnt;
        for  (cnt = 0;  cnt < sv->memb_cnt;  cnt++)
                free(sv->memb_list[cnt]);
        free((char *) sv->memb_list);
}

/* One day, my boy, all these will be stringvecs */

char **stringvec_chararray(struct stringvec *sv)
{
        char    **result = (char **) malloc((unsigned) (sv->memb_cnt + 1) * sizeof(char *)), **rp;
        int     cnt;

        if  (!result)
                ABORT_NOMEM;

        rp = result;
        for  (cnt = 0;  cnt < sv->memb_cnt;  cnt++)
                *rp++ = stracpy(sv->memb_list[cnt]);
        *rp = 0;
        return  result;
}
