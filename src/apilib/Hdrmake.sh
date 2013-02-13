#! /bin/sh
#
#   Copyright 2009 Free Software Foundation, Inc.
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */
#
# Generate gbatch.h from header files keeping extraneous stuff at bay
#

CONFIG=${1:-../../config.h}
HDRS=${2:-../hdrs}

rm -f gbatch.h

cp gbatch.pre gbatch.h
chmod 644 gbatch.h

cat - <<\% >>gbatch.h

/* This section built from config.h */

%
cat $CONFIG >>gbatch.h

# Copy headers but skip function defs

for file in defaults bjparam btconst btmode timecon
do
        cat - <<EOF >>gbatch.h

/* This section built from $file.h */

EOF
        sed -e '/extern /d' $HDRS/$file.h >>gbatch.h
done

# Copy error codes only out of xbapi_int.h and rename

sed '1,/ERRSTART/d
s/XB_/GBATCH_/g
/ERREND/,$d' $HDRS/xbapi_int.h >>gbatch.h

cat gbatch.post >>gbatch.h
