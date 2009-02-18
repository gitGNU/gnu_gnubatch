/* statenum.h -- Prompts for mode display

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

#define	BS_PPF		2		/* Privilege header */
#define	BS_PPN		3		/* Privilege next */
#define	BS_MJ		5		/* Mode Jobs */
#define	BS_MV		6		/* Mode vars */
#define	BS_MDU		10		/* Name for `user' in mode display */
#define	BS_MDG		11		/* Name for `group' */
#define	BS_MDO		12		/* Name for `others' */
#define	BS_MDFSEP	13		/* Separator for 2nd and subs */
#define	BS_MDSSEP	14		/* Separator for 2nd and subs */

#define	BS_MOREA	20		/* `More above' message */
#define	BS_MOREB	21		/* `More below' message */

#define	BS_LSTATEV	1		/* List view state */
#define	BS_LSTATEI	2		/* List change state */
#define	BS_VPRIVS	3		/* View Privileges */
#define	BS_EPRIVS	4		/* Edit Privileges */
#define	BS_PCONFHLP	5		/* Confirmation help */
#define	BS_VMODES	6		/* View Modes */
#define	BS_EMODES	7		/* Edit Modes */
#define	BS_MCONFHLP	8		/* Confirmation help */

/*
 *	Privilege state codes
 */

#define	BS_PINIT	399		/* Privilege init */
#define	BS_ABBREV	10		/* Add this for abbreviated text */
#define	BS_RDADMIN	400		/* Read admin file */
#define	BS_WRADMIN	401		/* Write admin file */
#define	BS_CREATE	402		/* Create entry */
#define	BS_SPCREATE	403		/* Special create */
#define	BS_STOP		404		/* Stop the thing */
#define	BS_UMASK	405		/* Change defaults */

#define	BS_COLON	419		/* Stuff after descr */
#define	BS_PNO		420		/* No for priv */
#define	BS_PYES		421		/* Yes for priv */
#define	BS_PDEFAULT	422		/* Default is... */
#define	BS_PUSER	423		/* User may... */
#define	BS_PCOPYOVER	424		/* Copy to everyone else... */
#define	BS_MCOPYOVER	425		/* Ditto for modes */

/*
 *	Mode state codes
 */

#define	BS_MINIT	449		/* Mode init */
#define	BS_READ		450		/* Read var/job */
#define	BS_WRITE	451		/* Write var/job */
#define	BS_SHOW		452		/* Show var/job */
#define	BS_RDMODE	453		/* Read mode */
#define	BS_WRMODE	454		/* Write mode */
#define	BS_UTAKE	455		/* Assume ownership */
#define	BS_GTAKE	456		/* Assume group ownership */
#define	BS_UGIVE	457		/* Give away */
#define	BS_GGIVE	458		/* Give away group */
#define	BS_DELETE	459		/* Delete item */
#define	BS_KILL		460		/* Kill jobs */

#define	BS_MNO		470		/* No for priv */
#define	BS_MYES		471		/* Yes for priv */

#define	BS_MJHDR	480		/* Header text - jobs */
#define	BS_MVHDR	481		/* Header text - vars */

#define	BS_MU		490		/* Header text - users */
#define	BS_MG		491		/* Header text - groups */
#define	BS_MO		492		/* Header text - others */

#define	BS_DELJPREFIX	910		/* Prefix for unqueue job */
#define	BS_DELCPREFIX	911		/* Prefix for unqueue file */

#define	BS_STDIN	96		/* Standard input message */

#define	BS_EXITCODENAME	98		/* Exit code name */
#define	BS_SIGNALNAME	99		/* Signal name */

#define	BS_JSTATE	1		/* Display job list */
#define	BS_VSTATE	2		/* Display var list */
#define	BS_CJDELSTATE	3		/* Confirm delete job */
#define	BS_CVDELSTATE	4		/* Confirm delete var */
#define	BS_JMODESTATE	5		/* Set job modes */
#define	BS_VMODESTATE	6		/* Set var modes */
#define	BS_INTSTATE	7		/* View job */
#define	BS_JCVDSTATE	8		/* Set conditions delete */
#define	BS_JSVDSTATE	9		/* Set variables delete */

#define	BS_JVARC	10		/* Set conditions */
#define	BS_JVARS	11		/* Set settings */

#define	BS_JSFSTATE	12		/* Set flags on var assigns */

#define	BS_CISTATE	13		/* Cmd interp */
#define	BS_CCIDELSTATE	14		/* Delete one */
#define	BS_MWFSTATE	15		/* Edit mail/write flags */

#define	BS_JARGS	16		/* Editing arguments */
#define	BS_JENV		17		/* Editing environment */
#define	BS_JREDIR	18		/* Editing redirections */
#define	BS_PENV		19		/* Editing process environment */

/*
 *	The following relates to the time/date/repeat code mostly
 *	in wtimes.c
 */

#define	BS_DSTATE	20		/* Setting date */

#define	BS_HOLSTATE	25		/* Setting holidays */

/*
 *	Message codes for normal/error/abort/cancelled
 */

#define	MSG_COMP	50
#define	MSG_EHALTED	51
#define	MSG_ABORTED	52
#define	MSG_CANCELLED	53

/*
 *	The following relate to the job editing stuff in
 *	bq_jlist.c and bq_jobops.c
 */

#define	BS_MOREJA	200		/* More jobs above */
#define	BS_MOREJB	201		/* More jobs below */
#define	BS_PROGLST	202		/* Job progress */
#define	BS_SPROGLST	203		/* Job progress for setting */
#define	BS_CONDSTR	204		/* Condition present */
#define	BS_KILLLST	205		/* Kill types */

#define	BS_EXPORTMARK	207		/* Exported var marker */
#define	BS_LOCRUN	208		/* Remote job run locally */

#define	BS_UNKNOWNCI	210		/* Unknown command interpreter */

#define	BS_TITLE	220		/* New title */
#define	BS_PTY		221		/* New priority */
#define	BS_LL		222		/* New load level */
#define	BS_CI		223		/* New command interp */

#define	BS_COMPNAMES	230		/* Comparison names */
#define	BS_NEWCOND	231		/* Help code (new) */
#define	BS_UPDCOND	232		/* Help code (update) */
#define	BS_CCRITMARK	233		/* Marker for critical condition */

#define	BQJ_USERP	235		/* Help for user in job list */
#define	BQJ_GROUPP	236		/* Help for group in job list */
#define	BQJ_CVNP	237		/* Help for comparison var name */
#define	BQJ_SVNP	238		/* Help for set var name */
#define	BQJ_CONFDEL	239		/* Confirm delete */

#define	BS_ENTRY_DEF_SV		241	/* Code for default flag entry */
#define	BS_REVERSE_DEF_SV	242	/* Code for default flag reverse */
#define	BS_EXIT_DEF_SV		243	/* Code for default flag ok */
#define	BS_ERROR_DEF_SV		244	/* Code for default flag error */
#define	BS_ABORT_DEF_SV		245	/* Code for default flag abort */
#define	BS_CANC_DEF_SV		246	/* Code for default flag canc */

#define	BQI_EOFMSG		250
#define	BQI_EOPMSG		251

#define	BTQ_VALP		260
#define	BTQ_UVALP		261

#define	BS_ASSNAMES	270
#define	BS_SETYES	271
#define	BS_SETNO	272
#define	BS_NEWASS	273
#define	BS_UPDASS	274
#define	BS_SETCRIT	275

#define	BS_MFLAG	280
#define	BS_WFLAG	281
#define	BS_MWSETYES	282
#define	BS_MWSETNO	283

#define	BS_UDDIR	290
#define	BS_UDXF		291
#define	BS_UDJF		292
#define	BS_UDPROMPT	293

/*
 *	Prompts for variable mangling operations
 */

#define	BQV_TITLE	330
#define	BQV_NAMEP	331
#define	BQV_VALP	332
#define	BQV_COMMP	333

#define	BQV_CONFDEL	335

#define	BQV_CONSTP	340

#define	BQV_USERP	360
#define	BQV_GROUPP	361
#define	BQV_RENAMEP	362

/*
 *	Command interp stuff
 */

#define	BS_CINAME	500
#define	BS_CIPATH	501
#define	BS_CILL		502
#define	BS_CONFDCI	503
#define	BS_CINICE	504
#define	BS_CIARGS	505

#define	BS_CONSTVAL	508

#define	BJA_ARGVAL	600
#define	BJA_ENVNAM	601
#define	BJA_ENVVAL	602
#define	BJA_FNAME	603
#define	BJA_FD1		604
#define	BJA_FD2		605

#define	BS_MOVING	650
#define	BS_COPYING	651

#define	BS_REDIRACT	660
#define	BS_STDINNAMES	661

#define	BS_PDIRECTORY	670
#define	BS_PUMASK	671
#define	BS_PULIMIT	672
#define	BS_NEXIT	673
#define	BS_EEXIT	674
#define	BS_DATIMEP	675
#define	BS_EXPORTTYPE	676

#define	BS_SFORWMSG	680
#define	BS_SBACKWMSG	681
#define	BS_SEARCHSTRING	680

/*
 *	Prompts for time/date etc
 */

#define	BTS_STIME	1000		/* Set time for... */
#define	BTS_WEEKDAYS	1001
#define	BTS_MONTHS	1002
#define	BTS_RDELETE	1003
#define	BTS_RRETAIN	1004

#define	BTS_REPO	1010		/* Repeat options... */
#define	BTS_REPUNIT	1011		/* Units of time */
#define	BTS_DAYPR	1012		/* Day (for months) */
#define	BTS_AVOID	1015		/* Avoiding */
#define	BTS_NDAY	1016		/* Not this day */
#define	BTS_NPOSS	1020		/* Not possible options... */
#define	BTS_APRYES	1025		/* Set time */
#define	BTS_APRNO	1026		/* Don't set it */

#define	BTS_DEFABASE	1050		/* Avoid Sunday to 1057 */

/*
 *	Help stuff
 */

#define	BTQ_TDIGSH	1000		/* Help for digits */
#define	BTQ_TDIGSM	1001		/* Help for digits */
#define	BTQ_TDW		1002		/* Help for day of week */
#define	BTQ_TDM		1003		/* Help for day of month */
#define	BTQ_TMN		1004		/* Help for month */
#define	BTQ_TYR		1005		/* Help for year */
#define	BTQ_ROOPTS	1010		/* Help for repeat options */
#define	BTQ_INTVAL	1011		/* Help for interval */
#define	BTQ_DAYNO	1012		/* Help for day number */
#define	BTQ_AVHELP	1015		/* Help for avoid days */
#define	BTQ_SKIP	1020		/* Help for skip option */

#define	BTR_CONFIRM	100		/* Confirm message - btstop */

/*
 *	Headers for fields in btjlist
 */

#define	BSJ_FORMAT_A	2000
#define	BSJ_FORMAT_B	2001
#define	BSJ_FORMAT_C	2002
#define	BSJ_FORMAT_D	2003
#define	BSJ_FORMAT_E	2004
#define	BSJ_FORMAT_F	2005
#define	BSJ_FORMAT_G	2006
#define	BSJ_FORMAT_H	2007
#define	BSJ_FORMAT_I	2008
#define	BSJ_FORMAT_J	2009
#define	BSJ_FORMAT_K	2010
#define	BSJ_FORMAT_L	2011
#define	BSJ_FORMAT_M	2012
#define	BSJ_FORMAT_N	2013
#define	BSJ_FORMAT_O	2014
#define	BSJ_FORMAT_P	2015
#define	BSJ_FORMAT_Q	2016
#define	BSJ_FORMAT_R	2017
#define	BSJ_FORMAT_S	2018
#define	BSJ_FORMAT_T	2019
#define	BSJ_FORMAT_U	2020
#define	BSJ_FORMAT_V	2021
#define	BSJ_FORMAT_W	2022
#define	BSJ_FORMAT_X	2023
#define	BSJ_FORMAT_Y	2024
#define	BSJ_FORMAT_Z	2025
#define	BSJ_FORMAT_a	2026
#define	BSJ_FORMAT_b	2027
#define	BSJ_FORMAT_c	2028
#define	BSJ_FORMAT_d	2029
#define	BSJ_FORMAT_e	2030
#define	BSJ_FORMAT_f	2031
#define	BSJ_FORMAT_g	2032
#define	BSJ_FORMAT_h	2033
#define	BSJ_FORMAT_i	2034
#define	BSJ_FORMAT_j	2035
#define	BSJ_FORMAT_k	2036
#define	BSJ_FORMAT_l	2037
#define	BSJ_FORMAT_m	2038
#define	BSJ_FORMAT_n	2039
#define	BSJ_FORMAT_o	2040
#define	BSJ_FORMAT_p	2041
#define	BSJ_FORMAT_q	2042
#define	BSJ_FORMAT_r	2043
#define	BSJ_FORMAT_s	2044
#define	BSJ_FORMAT_t	2045
#define	BSJ_FORMAT_u	2046
#define	BSJ_FORMAT_v	2047
#define	BSJ_FORMAT_w	2048
#define	BSJ_FORMAT_x	2049
#define	BSJ_FORMAT_y	2050
#define	BSJ_FORMAT_z	2051

/*
 *	Ditto btvlist - not nearly so many
 */

#define	BSV_FORMAT_C	2062
#define	BSV_FORMAT_E	2064
#define	BSV_FORMAT_G	2066
#define	BSV_FORMAT_M	2072
#define	BSV_FORMAT_N	2073
#define	BSV_FORMAT_U	2080
#define	BSV_FORMAT_V	2081

/*
 *	Prompts for scheduler
 */

#define	BSD_LL		1		/* Load level */
#define	BSD_CL		3		/* Current load */
#define	BSD_LJ		5		/* Log jobs */
#define	BSD_LV		7		/* Log vars */
#define	BSD_MACH	9		/* Machine name */

#define	BSD_LEVENTS	300
#define	BSD_LSOURCES	301
#define	BSD_UNNAMED	302
#define	BSD_EXPORTMSG	303
#define	BSD_LOCALMSG	304

extern	short	helpnstate(const short);
