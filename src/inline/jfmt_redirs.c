/* jfmt_redirs.c -- format redirections for job display

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

JFORMAT(fmt_redirs)
{
        fmt_t  lng = 0;
#ifdef  CHARSPRINTF
        int     cnt;
#endif
        if  (isreadable)  {
                unsigned        rc;
                for  (rc = 0;  rc < jp->h.bj_nredirs;  rc++)  {
                        CRedirRef       rp = REDIR_OF(jp, rc);
                        if  (rc != 0)
                                bigbuff[lng++] = ',';
                        switch  (rp->action)  {
                        case  RD_ACT_RD:
#ifdef  CHARSPRINTF
                                if  (rp->fd != 0)  {
                                        sprintf(&bigbuff[lng], "%d", rp->fd);
                                        cnt = strlen(&bigbuff[lng]);
                                        lng += cnt;
                                }
#else
                                if  (rp->fd != 0)
                                        lng += sprintf(&bigbuff[lng], "%d", rp->fd);
#endif
                                bigbuff[lng++] = '<';
                                break;
                        case  RD_ACT_WRT:
#ifdef  CHARSPRINTF
                                if  (rp->fd != 1)  {
                                        sprintf(&bigbuff[lng], "%d", rp->fd);
                                        cnt = strlen(&bigbuff[lng]);
                                        lng += cnt;
                                }
#else
                                if  (rp->fd != 1)
                                        lng += sprintf(&bigbuff[lng], "%d", rp->fd);
#endif
                                bigbuff[lng++] = '>';
                                break;
                        case  RD_ACT_APPEND:
#ifdef  CHARSPRINTF
                                if  (rp->fd != 1)  {
                                        sprintf(&bigbuff[lng], "%d", rp->fd);
                                        cnt = strlen(&bigbuff[lng]);
                                        lng += cnt;
                                }
#else
                                if  (rp->fd != 1)
                                        lng += sprintf(&bigbuff[lng], "%d", rp->fd);
#endif
                                bigbuff[lng++] = '>';
                                bigbuff[lng++] = '>';
                                break;
                        case  RD_ACT_RDWR:
#ifdef  CHARSPRINTF
                                if  (rp->fd != 0)  {
                                        sprintf(&bigbuff[lng], "%d", rp->fd);
                                        cnt = strlen(&bigbuff[lng]);
                                        lng += cnt;
                                }
#else
                                if  (rp->fd != 0)
                                        lng += sprintf(&bigbuff[lng], "%d", rp->fd);
#endif
                                bigbuff[lng++] = '<';
                                bigbuff[lng++] = '>';
                                break;
                        case  RD_ACT_RDWRAPP:
#ifdef  CHARSPRINTF
                                if  (rp->fd != 0)  {
                                        sprintf(&bigbuff[lng], "%d", rp->fd);
                                        cnt = strlen(&bigbuff[lng]);
                                        lng += cnt;
                                }
#else
                                if  (rp->fd != 0)
                                        lng += sprintf(&bigbuff[lng], "%d", rp->fd);
#endif
                                bigbuff[lng++] = '<';
                                bigbuff[lng++] = '>';
                                bigbuff[lng++] = '>';
                                break;
                        case  RD_ACT_PIPEO:
#ifdef  CHARSPRINTF
                                if  (rp->fd != 1)  {
                                        sprintf(&bigbuff[lng], "%d", rp->fd);
                                        cnt = strlen(&bigbuff[lng]);
                                        lng += cnt;
                                }
#else
                                if  (rp->fd != 1)
                                        lng += sprintf(&bigbuff[lng], "%d", rp->fd);
#endif
                                bigbuff[lng++] = '|';
                                break;
                        case  RD_ACT_PIPEI:
#ifdef  CHARSPRINTF
                                if  (rp->fd != 0)  {
                                        sprintf(&bigbuff[lng], "%d", rp->fd);
                                        cnt = strlen(&bigbuff[lng]);
                                        lng += cnt;
                                }
#else
                                if  (rp->fd != 0)
                                        lng += sprintf(&bigbuff[lng], "%d", rp->fd);
#endif
                                bigbuff[lng++] = '<';
                                bigbuff[lng++] = '|';
                                break;
                        case  RD_ACT_CLOSE:
#ifdef  CHARSPRINTF
                                if  (rp->fd != 1)  {
                                        sprintf(&bigbuff[lng], "%d", rp->fd);
                                        cnt = strlen(&bigbuff[lng]);
                                        lng += cnt;
                                }
#else
                                if  (rp->fd != 1)
                                        lng += sprintf(&bigbuff[lng], "%d", rp->fd);
#endif
                                bigbuff[lng++] = '>';
                                bigbuff[lng++] = '&';
                                bigbuff[lng++] = '-';
                                continue;
                        case  RD_ACT_DUP:
#ifdef  CHARSPRINTF
                                if  (rp->fd != 1)  {
                                        sprintf(&bigbuff[lng], "%d", rp->fd);
                                        cnt = strlen(&bigbuff[lng]);
                                        lng += cnt;
                                }
                                sprintf(&bigbuff[lng], ">&%d", rp->arg);
                                cnt = strlen(&bigbuff[lng]);
                                lng += cnt;
#else
                                if  (rp->fd != 1)
                                        lng += sprintf(&bigbuff[lng], "%d", rp->fd);
                                lng += sprintf(&bigbuff[lng], ">&%d", rp->arg);
#endif
                                continue;
                        }
#ifdef  CHARSPRINTF
                        strlen(strcpy(&bigbuff[lng], &jp->bj_space[rp->arg]));
                        cnt = strlen(&bigbuff[lng]);
                        lng += cnt;
#else
                        lng += strlen(strcpy(&bigbuff[lng], &jp->bj_space[rp->arg]));
#endif
                }
        }
        return  lng;
}
