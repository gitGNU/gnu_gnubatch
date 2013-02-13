/* incl_sig.h -- attempt at a common signal-handling routine

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

#include <signal.h>

#ifdef  HAVE_SIGSET
#define signal  sigset
#endif

#if     !(defined(HAVE_SIGSET) || (defined(HAVE_SIGVEC) && defined(SV_INTERRUPT)) || defined(HAVE_SIGVECTOR) || defined(HAVE_SIGACTION))
#define UNSAFE_SIGNALS
#endif

#ifdef  HAVE_SIGACTION
#define STRUCT_SIG
#define sigact_routine  sigaction
#define sigstruct_name  sigaction
#define sighandler_el   sa_handler
#define sigmask_clear(X)        sigemptyset(&X.sa_mask)
#define sigflags_el     sa_flags
#define SIGVEC_INTFLAG  0
#ifdef  SA_NODEFER
#define SIGACT_INTSELF  SA_NODEFER
#else
#define SIGACT_INTSELF  0
#endif
#elif   defined(HAVE_SIGVECTOR)
#define STRUCT_SIG
#define sigact_routine  sigvector
#define sigstruct_name  sigvec
#define sighandler_el   sv_handler
#define sigmask_clear(X)        X.sv_mask = 0
#define sigflags_el     sv_flags
#define SIGVEC_INTFLAG  0
#define SIGACT_INTSELF  0
#elif   defined(HAVE_SIGVEC) && defined(SV_INTERRUPT)
#define STRUCT_SIG
#define sigact_routine  sigvec
#define sigstruct_name  sigvec
#define sighandler_el   sv_handler
#define sigmask_clear(X)        X.sv_mask = 0
#define sigflags_el     sv_flags
#define SIGVEC_INTFLAG  SV_INTERRUPT
#define SIGACT_INTSELF  0
#else
#define sigvec_routine  cockup in incl_sig
#endif
#ifdef  OS_FREEBSD
#define SIGCLD  SIGCHLD
#endif
