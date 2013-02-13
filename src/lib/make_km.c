/* make_km.c -- setup keymap for function key interpretation (Curses clients)

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
#include "keymap.h"
#include "errnums.h"

static  char    Filename[] = __FILE__;

static  struct  keymap_vec      gen_map[MAPSIZE];
struct  keymap_vec      *curr_map;
struct  state_map       *state_map;
static  int     nstates, maxkchars;
int     keyerrors;

static  void  append_rest(struct keymap_sparse **kvp, const int n, const char *text, const int length, const int value)
{
        struct  keymap_sparse   *kp, **pp;
        int     ch = text[n] & (MAPSIZE-1);

        for  (pp = kvp;  (kp = *pp);  pp = &kp->ks_next)
                if  (ch == kp->ks_char)
                        goto  found;

        /* Run off end, just allocate a new one (easy!)  */

        if  ((kp = (struct keymap_sparse *)malloc(sizeof(struct keymap_sparse))) == (struct  keymap_sparse *) 0)
             ABORT_NOMEM;

        kp->ks_char = (char) ch;
        kp->ks_next = (struct  keymap_sparse  *) 0;
        kp->ks_link = (struct  keymap_sparse  *) 0;

        if  (n + 1 >= length)  {
                kp->ks_type = KV_CHAR;
                kp->ks_value = (SHORT) value;
        }
        else  {
                kp->ks_type = KV_SMAP;
                append_rest(&kp->ks_link, n+1, text, length, value);
        }
        *pp = kp;
        return;

found:
        if  (n + 1 >= length)  {
                if  (kp->ks_type & KV_CHAR && kp->ks_value != value)  {
                        disp_arg[0] = text[n];
                        disp_arg[1] = value;
                        disp_arg[2] = kp->ks_value;
                        disp_arg[3] = n;
                        print_error($E{Key sequence error});
                        keyerrors++;
                        return;
                }
                kp->ks_type |= KV_CHAR;
                kp->ks_value = (SHORT) value;
        }
        else  {
                kp->ks_type |= KV_SMAP;
                append_rest(&kp->ks_link, n+1, text, length, value);
        }
}

static void insert_key(struct keymap_vec *kv, const char *text, const int length, const int value, const int ecode)
{
        if  (length > maxkchars)
                maxkchars = length;

        if  (length > 1)  {
                kv->kv_type |= KV_SMAP;
                append_rest(&kv->kv_link, 1, text, length, value);
        }
        else  if  (kv->kv_type & KV_CHAR  &&  kv->kv_value != value)  {
                disp_arg[0] = text[0];
                disp_arg[1] = value;
                disp_arg[2] = kv->kv_value;
                print_error(ecode);
                keyerrors++;
        }
        else  {
                kv->kv_type |= KV_CHAR;
                kv->kv_value = (SHORT) value;
        }
}

void  insert_global_key(const char *text, const int length, const int value)
{
        insert_key(&gen_map[text[0] & (MAPSIZE-1)], text, length, value, $E{Global key error});
}

void  map_dup(struct keymap_sparse **ksp)
{
        struct  keymap_sparse  *newp;

        if  ((newp = (struct keymap_sparse *)malloc(sizeof(struct keymap_sparse))) == (struct keymap_sparse *) 0)
             ABORT_NOMEM;
        *newp = **ksp;
        *ksp = newp;
        if  (newp->ks_next)
                map_dup(&newp->ks_next);
        if  (newp->ks_link)
                map_dup(&newp->ks_link);
}

void  insert_state_key(const int state, const char *text, const int length, const int value)
{
        struct  state_map       *sm;
        int     i;

        disp_arg[3] = state;            /*  In case of error  */

        if  (state_map)  {
                for  (sm = state_map;  sm < &state_map[nstates];  sm++)
                        if  (sm->state_number == state)  {
                                insert_key(&sm->state_map[text[0] & (MAPSIZE-1)], text, length, value, $E{State key error});
                                return;
                        }

                nstates++;
                if  ((state_map = (struct state_map *)
                      realloc((char *) state_map, (unsigned)(nstates * sizeof(struct state_map))))
                     == (struct state_map *) 0)
                        ABORT_NOMEM;
                sm = &state_map[nstates-1];
        }
        else  {
                nstates = 1;
                if  ((sm = state_map = (struct state_map *) malloc(sizeof(struct state_map))) == (struct state_map *) 0)
                        ABORT_NOMEM;
        }

        /* Initialise to general map */

        sm->state_number = state;
        for  (i = 0;  i < MAPSIZE;  i++)  {
                sm->state_map[i] = gen_map[i];
                if  (gen_map[i].kv_type & KV_SMAP)
                        map_dup(&sm->state_map[i].kv_link);
        }

        insert_key(&sm->state_map[text[0] & (MAPSIZE-1)], text, length, value, $E{State key error});
}

void  select_state(const int state)
{
        struct  state_map       *sm;

        disp_arg[3] = state;            /*  In case of error  */

        if  (state_map)  {
                for  (sm = state_map;  sm < &state_map[nstates];  sm++)
                        if  (sm->state_number == state)  {
                                curr_map = sm->state_map;
                                return;
                        }
        }

        /* Change for GNUbatch view modes....
           Don't object if no extra keys for this state.  */

        curr_map = gen_map;
}

void  reset_state()
{
        curr_map = gen_map;
}
