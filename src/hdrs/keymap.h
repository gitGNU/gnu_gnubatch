/* keymap.h -- map used to remember function key sequences

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

#define MAXTBUF 7       /*  Maximum length character sequence  */

/* Only bother about chars 0 to 127 */

#define MAPSIZE 128

struct  keymap_sparse   {
        unsigned  char  ks_type;                /*  Type etc see below  */
        char    ks_char;                        /*  ... we're looking for */
        SHORT   ks_value;                       /*  value if terminator */
        struct  keymap_sparse   *ks_link;       /*  to next char  */
        struct  keymap_sparse   *ks_next;       /*  in sparse map  */
};

struct  keymap_vec      {
        unsigned  char  kv_type;                /*  Type etc see below  */
        SHORT   kv_value;                       /*  value if terminator  */
        struct  keymap_sparse   *kv_link;       /*  to next char in seq  */
};

/* k[vs]_type values - bits.  NB both may be set for cases (e.g.
   escape) where the character means something on its own AND as
   a prefix to another sequence. */

#define KV_CHAR         1       /*  Just return the char  */
#define KV_SMAP         2       /*  "Sparse map" for next char(s) */

extern  struct  keymap_vec      *curr_map;       /*  "Current" map  */

struct  state_map       {
        int     state_number;                   /*  State as in nK  */
        struct  keymap_vec  state_map[MAPSIZE]; /*  Corresponding lookup  */
};

extern  struct  state_map       *state_map;
