/* btuser.h -- user permission structure

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

#ifdef	__cplusplus
struct  Btuser	{
#else
typedef	struct	{
#endif
	unsigned  char	btu_isvalid,	/* Valid user id */
			btu_minp, 	/* Minimum priority  */
			btu_maxp,	/* Maximum priority  */
			btu_defp;	/* Default priority  */
	int_ugid_t	btu_user;	/* User id */
	
	unsigned  long	btu_priv;	/* Privileges */
	
	unsigned  short	btu_maxll;	/* Max load level */
	unsigned  short	btu_totll;	/* Max total load level */
	unsigned  short	btu_spec_ll;	/* Non-std jobs load level */
	unsigned  short	btu_jflags[3];	/* Flags for jobs */
	unsigned  short	btu_vflags[3];	/* Flags for variables */
#ifdef	__cplusplus
};
#else
} Btuser;
#endif

//	Initial minimum, maximum and default priorities

#define	U_DF_MINP	100
#define	U_DF_MAXP	200
#define	U_DF_DEFP	150

#define	U_DF_MAXLL	1000
#define	U_DF_TOTLL	10000
#define	U_DF_SPECLL	1000


//	Default privileges

#define	U_DF_UJ		(BTM_READ|BTM_WRITE|BTM_SHOW|BTM_RDMODE|BTM_WRMODE|BTM_UGIVE|BTM_GGIVE|BTM_DELETE|BTM_KILL)
#define	U_DF_GJ		(BTM_READ|BTM_SHOW|BTM_RDMODE|BTM_GGIVE)

#define	U_DF_UV		(BTM_READ|BTM_WRITE|BTM_SHOW|BTM_RDMODE|BTM_WRMODE|BTM_UGIVE|BTM_GGIVE|BTM_DELETE)
#define	U_DF_GV		(BTM_READ|BTM_SHOW|BTM_RDMODE|BTM_GGIVE)

#define	U_DF_OJ		(BTM_SHOW|BTM_RDMODE)
#define	U_DF_OV		(BTM_SHOW|BTM_RDMODE)

#define	U_DF_PRIV	(BTM_UMASK|BTM_CREATE)

#ifdef	__cplusplus
const	int	xb_enquire(CString &, CString &, CString &);
const	int	xb_login(CString &, CString &, const char *, CString &);
const	int	xb_newgrp(CString &);
const	int	xb_logout();
const   int	getbtuser(Btuser FAR &, CString &, CString &);
#endif
