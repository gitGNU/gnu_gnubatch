/* incl_net.h -- De-messify network include files by trying all the options here

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

#include <netdb.h>
#include <sys/socket.h>
#ifdef  HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#if     defined(i386) && defined(__GNUC__)
#define NO_ASM
#endif
#include <netinet/in.h>
#include <arpa/inet.h>
#ifndef AF_INET
#define AF_INET 2
#endif
#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK ((in_addr_t) 0x7f000001) /* Inet 127.0.0.1.  */
#endif

extern  struct  servent *env_getserv(const char *, const int);
