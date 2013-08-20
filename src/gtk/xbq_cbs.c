/* xbq_cbs.c -- Generalised callback routines for gbch-xq

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
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <errno.h>
#include <pwd.h>
#include <gtk/gtk.h>
#include "incl_unix.h"
#include "incl_sig.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "incl_ugid.h"
#include "helpalt.h"
#include "files.h"
#include "btconst.h"
#include "timecon.h"
#include "btmode.h"
#include "bjparam.h"
#include "btjob.h"
#include "cmdint.h"
#include "btvar.h"
#include "btuser.h"
#include "shreq.h"
#include "statenums.h"
#include "errnums.h"
#include "ecodes.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "jvuprocs.h"
#include "xbq_ext.h"
#include "optflags.h"
#include "cfile.h"
#include "gtk_lib.h"
#include "stringvec.h"

static  char    Filename[] = __FILE__;

char    *execprog;

extern  struct  macromenitem    jobmacs[], varmacs[];

extern void  job_redisplay();
extern void  var_redisplay();
extern void  gen_qlist(struct stringvec *);
extern char *gen_jfmts();
extern char *gen_vfmts();

void  cb_viewopt()
{
        GtkWidget  *dlg, *hbox, *qcomb, *ucomb, *gcomb, *nullq, *remh, *confdel;
#ifdef  HAVE_LIBXML2
        GtkWidget  *xmlfmtw;
#endif
        char    *pr, **uglist, **up;
        int     cnt;
        struct  stringvec  possqs;

        pr = gprompt($P{xbtq viewopt dlgtit});
        dlg = gtk_dialog_new_with_buttons(pr,
                                          GTK_WINDOW(toplevel),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK,
                                          GTK_RESPONSE_OK,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL,
                                          NULL);
        free(pr);

        /* Queue prefix */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq viewopt queue lab}), FALSE, FALSE, DEF_DLG_HPAD);
        gen_qlist(&possqs);
        qcomb = gtk_combo_box_entry_new_text();
        if  (jobqueue  &&  strlen(jobqueue) != 0)
                gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(qcomb))), jobqueue);
        for  (cnt = 0;  cnt < stringvec_count(possqs);  cnt++)
                gtk_combo_box_append_text(GTK_COMBO_BOX(qcomb), stringvec_nth(possqs, cnt));
        stringvec_free(&possqs);
        gtk_box_pack_start(GTK_BOX(hbox), qcomb, FALSE, FALSE, DEF_DLG_HPAD);

        /* Possible users */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq viewopt users lab}), FALSE, FALSE, DEF_DLG_HPAD);
        uglist = gen_ulist((char *) 0);
        ucomb = gtk_combo_box_entry_new_text();
        if  (Restru  &&  strlen(Restru) != 0)
                gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ucomb))), Restru);
        gtk_combo_box_append_text(GTK_COMBO_BOX(ucomb), "");
        for  (up = uglist;  *up;  up++)
                gtk_combo_box_append_text(GTK_COMBO_BOX(ucomb), *up);
        freehelp(uglist);
        gtk_box_pack_start(GTK_BOX(hbox), ucomb, FALSE, FALSE, DEF_DLG_HPAD);

        /* Ditto groups */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq viewopt groups lab}), FALSE, FALSE, DEF_DLG_HPAD);
        uglist = gen_glist((char *) 0);
        gcomb = gtk_combo_box_entry_new_text();
        if  (Restrg  &&  strlen(Restrg) != 0)
                gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gcomb))), Restrg);
        gtk_combo_box_append_text(GTK_COMBO_BOX(gcomb), "");
        for  (up = uglist;  *up;  up++)
                gtk_combo_box_append_text(GTK_COMBO_BOX(gcomb), *up);
        freehelp(uglist);
        gtk_box_pack_start(GTK_BOX(hbox), gcomb, FALSE, FALSE, DEF_DLG_HPAD);

        /* Include null queue */

        nullq = gprompt_checkbutton($P{xbtq viewopt include null});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), nullq, FALSE, FALSE, DEF_DLG_VPAD);
        if  (!(Dispflags & DF_SUPPNULL))
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(nullq), TRUE);

        /* Remote hosts */

        remh = gprompt_checkbutton($P{xbtq viewopt remote hosts});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), remh, FALSE, FALSE, DEF_DLG_VPAD);
        if  (!(Dispflags & DF_LOCALONLY))
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remh), TRUE);

        /* Confirm delete */

        confdel = gprompt_checkbutton($P{xbtq viewopt confirm delete});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), confdel, FALSE, FALSE, DEF_DLG_VPAD);
        if  (Dispflags & DF_CONFABORT)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(confdel), TRUE);

#ifdef  HAVE_LIBXML2
        xmlfmtw = gprompt_checkbutton($P{xbtq viewopt save XML});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), xmlfmtw, FALSE, FALSE, DEF_DLG_VPAD);
        if  (xml_format)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(xmlfmtw), TRUE);
#endif

        gtk_widget_show_all(dlg);

        if  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const  char  *newq = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(qcomb))));
                const  char  *newu = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ucomb))));
                const  char  *newg = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gcomb))));
                if  (jobqueue)  {
                        free(jobqueue);
                        jobqueue = 0;
                }
                if  (strlen(newq) != 0)
                        jobqueue = stracpy(newq);
                if  (Restru)  {
                        free(Restru);
                        Restru = 0;
                }
                if  (strlen(newu) != 0)
                        Restru = stracpy(newu);
                if  (Restrg)  {
                        free(Restrg);
                        Restrg = 0;
                }
                if  (strlen(newg) != 0)
                        Restrg = stracpy(newg);

                if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(nullq)))
                        Dispflags &= ~DF_SUPPNULL;
                else
                        Dispflags |= DF_SUPPNULL;
                if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remh)))
                        Dispflags &= ~DF_LOCALONLY;
                else
                        Dispflags |= DF_LOCALONLY;
                if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(confdel)))
                        Dispflags |= DF_CONFABORT;
                else
                        Dispflags &= ~DF_CONFABORT;
#ifdef  HAVE_LIBXML2
                if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(xmlfmtw)))
                        xml_format = 1;
                else
                        xml_format = 0;
#endif
                Dirty = 1;
                Last_j_ser = Last_v_ser = 0;
                job_redisplay();
                var_redisplay();
        }
        gtk_widget_destroy(dlg);
}

extern char *encode_defcond(const int);
extern char *encode_defass(const int);
extern void  decode_defcond(char *);
extern void  decode_defass(char *);

void  cb_saveopts()
{
        PIDTYPE pid;
        static  char    *gtkprog;
        int     status;

        if  (!execprog)
                execprog = envprocess(EXECPROG);

        if  (!gtkprog)
                gtkprog = envprocess(GTKSAVE);

        if  ((pid = fork()) == 0)  {
                char    *jf = gen_jfmts(), *vf = gen_vfmts();
                char    digbuf[4], digbuf2[2], *argbuf[18 + 8 * MAXMACS + 2 * MAXCVARS + 2 * MAXSEVARS];
                char    **ap = argbuf;
                int     cnt;
                *ap++ = gtkprog; /* Arg 0 is the program we're running */
                *ap++ = "XBTQDISPOPT";
                digbuf[0] = Dispflags & DF_SUPPNULL? '0': '1';
                digbuf[1] = Dispflags & DF_LOCALONLY? '0': '1';
                digbuf[2] = Dispflags & DF_CONFABORT? '1': '0';
                digbuf[3] = '\0';
                *ap++ = digbuf;
                digbuf2[0] = xml_format? '1': '0';
                digbuf2[1] = '\0';
                *ap++ = "XBTRXMLFMT";                   /* I did mean that to keep sync with it */
                *ap++ = digbuf2;
                *ap++ = "XBTQDISPUSER";
                *ap++ = Restru? Restru: "-";
                *ap++ = "XBTQDISPGROUP";
                *ap++ = Restrg? Restrg: "-";
                *ap++ = "XBTQDISPQ";
                *ap++ = jobqueue? jobqueue: "-";
                *ap++ = "XBTQJOBFLD";
                *ap++ = jf;
                *ap++ = "XBTQVARFLD";
                *ap++ = vf;
                for  (cnt = 0;  cnt < MAXMACS;  cnt++)  {
                        struct macromenitem  *mi = &jobmacs[cnt];
                        char  nbuf[14];
                        sprintf(nbuf, "XBTQJOBMAC%d", cnt+1);
                        *ap++ = stracpy(nbuf);
                        *ap++ = mi->cmd? mi->cmd: "-";
                        sprintf(nbuf, "XBTQJOBMACD%d", cnt+1);
                        *ap++ = stracpy(nbuf);
                        *ap++ = mi->descr? mi->descr: "-";
                }
                for  (cnt = 0;  cnt < MAXMACS;  cnt++)  {
                        struct macromenitem  *mi = &varmacs[cnt];
                        char  nbuf[14];
                        sprintf(nbuf, "XBTQVARMAC%d", cnt+1);
                        *ap++ = stracpy(nbuf);
                        *ap++ = mi->cmd? mi->cmd: "-";
                        sprintf(nbuf, "XBTQVARMACD%d", cnt+1);
                        *ap++ = stracpy(nbuf);
                        *ap++ = mi->descr? mi->descr: "-";
                }
                for  (cnt = 0;  cnt < MAXCVARS;  cnt++)  {
                        char    *cv = encode_defcond(cnt);
                        if  (cv)  {
                                char    nbuf[20];
                                sprintf(nbuf, "XBTQDEFC%d", cnt);
                                *ap++ = stracpy(nbuf);
                                *ap++ = cv;
                        }
                }
                for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
                        char    *cv = encode_defass(cnt);
                        if  (cv)  {
                                char    nbuf[20];
                                sprintf(nbuf, "XBTQDEFA%d", cnt);
                                *ap++ = stracpy(nbuf);
                                *ap++ = cv;
                        }
                }
                *ap = 0;
                execv(execprog, argbuf);
                exit(E_SETUP);
        }

        if  (pid < 0)  {
                doerror($EH{saveopts cannot fork});
                return;
        }
#ifdef  HAVE_WAITPID
        while  (waitpid(pid, &status, 0) < 0)
                ;
#else
        while  (wait(&status) != pid)
                ;
#endif
        if  (status != 0)  {
                if  ((status & 127) != 0)  {
                        disp_arg[0] = status & 127;
                        doerror($EH{saveopts crashed});
                }
                else  {
                        int     msg;

                        disp_arg[0] = status >> 8;
                        switch  (disp_arg[0])  {
                        default:
                                msg = $EH{saveopts unknown exit};
                                break;
                        case  E_NOMEM:
                                msg = $EH{saveopts no memory};
                                break;
                        case  E_SETUP:
                                msg = $EH{saveopts bad setup};
                                break;
                        case  E_NOPRIV:
                                msg = $EH{saveopts no write};
                                break;
                        case  E_USAGE:
                                msg = $EH{saveopts usage};
                                break;
                        case  E_BTEXEC2:
                                msg = $EH{saveopts no exec};
                                break;
                        }
                        doerror(msg);
                }
                return;

        }
        Dirty = 0;
}

extern  USHORT  *def_jobflds, *def_varflds;
extern  int     ndef_jobflds, ndef_varflds;

int  parse_fldarg(char *arg, USHORT **list)
{
        char  *cp, *np;
        int     result = 1;
        USHORT  *lp;

        /* Count bits */

        for  (cp = arg;  (np = strchr(cp, ','));  cp = np+1)
                result++;

        *list = (USHORT *) malloc((unsigned) (result * sizeof(USHORT)));
        if  (!*list)
                ABORT_NOMEM;

        lp = *list;

        for  (cp = arg;  (np = strchr(cp, ','));  cp = np+1)
                *lp++ = atoi(cp);
        *lp = atoi(cp);

        free(arg);
        return  result;
}

void  loadmac(struct macromenitem *mlist, const int cnt, const char *jorv)
{
        char    nbuf[16], *cmd, *descr;
        sprintf(nbuf, "XBTQ%sMAC%d", jorv, cnt);
        cmd = optkeyword(nbuf);
        sprintf(nbuf, "XBTQ%sMACD%d", jorv, cnt);
        descr = optkeyword(nbuf);
        if  (cmd  &&  descr)  {
                mlist[cnt-1].cmd = cmd;
                mlist[cnt-1].descr = descr;
        }
        else  {
                if  (cmd)
                        free(cmd);
                if  (descr)
                        free(descr);
        }
}

/* Put this here to co-ordinate with above hopefully */

void  load_optfile()
{
        char    *arg;
        int     cnt;

        /* Display options */

        if  ((arg = optkeyword("XBTQDISPOPT")))  {
                if  (arg[0])  {
                        if  (arg[0] == '0')
                                Dispflags |= DF_SUPPNULL;
                        else
                                Dispflags &= ~DF_SUPPNULL;
                        if  (arg[1])  {
                                if  (arg[1] == '0')
                                        Dispflags |= DF_LOCALONLY;
                                else
                                        Dispflags &= ~DF_LOCALONLY;
                                if  (arg[2])  {
                                        if  (arg[2] == '0')
                                                Dispflags &= ~DF_CONFABORT;
                                        else
                                                Dispflags |= DF_CONFABORT;
                                }
                        }
                }
                free(arg);
        }

        if  ((arg = optkeyword("XBTRXMLFMT")))  {               /* Use this one to sync with xbtr */
                if  (arg[0])
                        xml_format = arg[0] != '0';
                free(arg);
        }

        if  ((arg = optkeyword("XBTQDISPUSER")))  {
                if  (strcmp(arg, "-") != 0)
                        Restru = arg;
                else
                        free(arg);
        }

        if  ((arg = optkeyword("XBTQDISPGROUP")))  {
                if  (strcmp(arg, "-") != 0)
                        Restrg = arg;
                else
                        free(arg);
        }

        if  ((arg = optkeyword("XBTQDISPQ")))  {
                if  (strcmp(arg, "-") != 0)
                        jobqueue = arg;
                else
                        free(arg);
        }

        if  ((arg = optkeyword("XBTQJOBFLD")))
                ndef_jobflds = parse_fldarg(arg, &def_jobflds);

        if  ((arg = optkeyword("XBTQVARFLD")))
                ndef_varflds = parse_fldarg(arg, &def_varflds);

        for  (cnt = 1;  cnt <= MAXMACS;  cnt++)  {
                loadmac(jobmacs, cnt, "JOB");
                loadmac(varmacs, cnt, "VAR");
        }

        for  (cnt = 0;  cnt < MAXCVARS;  cnt++)  {
                char    *arg, nbuf[20];
                sprintf(nbuf, "XBTQDEFC%d", cnt);
                if  ((arg = optkeyword(nbuf)))  {
                        decode_defcond(arg);
                        free(arg);
                }
        }
        for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
                char    *arg, nbuf[20];
                sprintf(nbuf, "XBTQDEFA%d", cnt);
                if  ((arg = optkeyword(nbuf)))  {
                        decode_defass(arg);
                        free(arg);
                }
        }

        close_optfile();
}

char    menutmpl[] =
"<ui>"
"<menubar name='MenuBar'>"
"<menu action='%cmacMenu'>"
"<placeholder name='%cmac%d'>"
"<menuitem action='%s'/>"
"</placeholder>"
"</menu>"
"</menubar>"
"</ui>";

extern void  jmacruncb(GtkAction *, struct macromenitem *);
extern void  vmacruncb(GtkAction *, struct macromenitem *);
extern  GtkUIManager    *ui;

void  setup_macro(const char jorv, struct macromenitem *mlist, const int macnum)
{
        GtkAction  *act;
        GtkActionGroup  *grp;
        char    anbuf[10], gnbuf[10];
        GString *uif;

        sprintf(anbuf, "%cm%d", jorv, macnum);
        act = gtk_action_new(anbuf, mlist[macnum-1].descr, NULL, NULL);
        g_signal_connect(act, "activate", jorv == 'j'? G_CALLBACK(jmacruncb): G_CALLBACK(vmacruncb), &mlist[macnum-1]);
        sprintf(gnbuf, "%cmg%d", jorv, macnum);
        grp = gtk_action_group_new(gnbuf);
        gtk_action_group_add_action(grp, act);
        g_object_unref(G_OBJECT(act));
        gtk_ui_manager_insert_action_group(ui, grp, 0);
        g_object_unref(G_OBJECT(grp));
        uif = g_string_new(NULL);
        g_string_printf(uif, menutmpl, jorv, jorv, macnum, anbuf);
        mlist[macnum-1].mergeid = gtk_ui_manager_add_ui_from_string(ui, uif->str, -1, NULL);
        g_string_free(uif, TRUE);
}

void  delete_macro(const char jorv, struct macromenitem *mlist, const int macnum)
{
        GList  *groups, *lp;
        char    gnbuf[10];

        sprintf(gnbuf, "%cmg%d", jorv, macnum);
        gtk_ui_manager_remove_ui(ui, mlist[macnum-1].mergeid);
        groups = gtk_ui_manager_get_action_groups(ui);
        for  (lp = groups;  lp;  lp = lp->next)  {
                GtkActionGroup  *grp = (GtkActionGroup *) lp->data;
                const  char  *nameg = gtk_action_group_get_name(grp);
                if  (strcmp(nameg, gnbuf) == 0)  {
                        gtk_ui_manager_remove_action_group(ui, grp);
                        break;
                }
        }
}

void  loadmacs(const char jorv, struct macromenitem *mlist)
{
        int     cnt;

        for  (cnt = 0;  cnt < MAXMACS;  cnt++)
                if  (mlist[cnt].cmd)
                        setup_macro(jorv, mlist, cnt+1);
}

char *get_macro_description()
{
        GtkWidget *dlg, *lab, *ent;
        char    *pr, *result = (char *) 0;

        pr = gprompt($P{xbtq macname dlg});
        dlg = gtk_dialog_new_with_buttons(pr, GTK_WINDOW(toplevel), GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
        free(pr);
        lab = gprompt_label($P{xbtq macname lab});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), lab, FALSE, FALSE, DEF_DLG_VPAD);
        ent = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), ent, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_widget_show_all(dlg);
        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const  char  *res = gtk_entry_get_text(GTK_ENTRY(ent));
                if  (strlen(res) == 0)  {
                        doerror($EH{xbtq macname null});
                        continue;
                }
                result = stracpy(res);
                break;
        }
        gtk_widget_destroy(dlg);
        return  result;
}

int  add_macro_to_list(const char *cmdtext, const char jorv, struct macromenitem *mlist)
{
        int     cnt;

        for  (cnt = 0;  cnt < MAXMACS;  cnt++)  {
                if  (!mlist[cnt].cmd)  {
                        char    *descr;
                        if  (!Confirm($PH{xbtq addmac to list}))
                                return  0;
                        if  (!(descr = get_macro_description()))
                                return  0;
                        mlist[cnt].cmd = stracpy(cmdtext);
                        mlist[cnt].descr = descr;
                        setup_macro(jorv, mlist, cnt+1);
                        Dirty = 1;
                        return  1;
                }
        }
        return  0;
}

struct  macupddata  {
        GtkWidget  *view;
        GtkListStore    *store;
        char    jorv;
        struct macromenitem *mlist;
};

GtkWidget *make_push_button(const int code)
{
        GtkWidget  *button, *hbox, *lab;
        button = gtk_button_new();
        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_container_add(GTK_CONTAINER(button), hbox);
        lab = gprompt_label(code);
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_BUTTON_PAD);
        return  button;
}

void  editmac(struct macupddata *mdata, const int cnt)
{
        int     macnum = cnt + 1;
        GtkWidget  *dlg, *hbox, *lab, *cmdw, *descrw;
        char    *pr;

        pr = gprompt($P{xbtq mac dlg});
        dlg = gtk_dialog_new_with_buttons(pr, GTK_WINDOW(toplevel), GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
        free(pr);

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        lab = gprompt_label($P{xbtq macdlg cmd});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);
        cmdw = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), cmdw, FALSE, FALSE, DEF_DLG_HPAD);

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        lab = gprompt_label($P{xbtq macdlg descr});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);
        descrw = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), descrw, FALSE, FALSE, DEF_DLG_HPAD);

        if  (mdata->mlist[cnt].cmd)  {
                gtk_entry_set_text(GTK_ENTRY(cmdw), mdata->mlist[cnt].cmd);
                gtk_entry_set_text(GTK_ENTRY(descrw), mdata->mlist[cnt].descr);
        }

        gtk_widget_show_all(dlg);

        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const  char  *newc = gtk_entry_get_text(GTK_ENTRY(cmdw));
                const  char  *newd = gtk_entry_get_text(GTK_ENTRY(descrw));
                GtkTreeIter  iter;

                if  (strlen(newc) == 0)  {
                        doerror($EH{xbtq empty macro command});
                        continue;
                }
                if  (strlen(newd) == 0)  {
                        doerror($EH{xbtq macname null});
                        continue;
                }

                if  (mdata->mlist[cnt].cmd)  {
                        delete_macro(mdata->jorv, mdata->mlist, macnum);
                        free(mdata->mlist[cnt].cmd);
                        free(mdata->mlist[cnt].descr);
                        gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(mdata->store), &iter, NULL, cnt);
                }
                else
                        gtk_list_store_append(mdata->store, &iter);

                mdata->mlist[cnt].cmd = stracpy(newc);
                mdata->mlist[cnt].descr = stracpy(newd);
                gtk_list_store_set(mdata->store, &iter, 0, cnt, 1, mdata->mlist[cnt].cmd, 2, mdata->mlist[cnt].descr, -1);
                setup_macro(mdata->jorv, mdata->mlist, macnum);
                Dirty = 1;
                break;
        }

        gtk_widget_destroy(dlg);
}

void  newmac_clicked(struct macupddata *mdata)
{
        int     cnt;
        for  (cnt = 0;  cnt < MAXMACS;  cnt++)
                if  (!mdata->mlist[cnt].cmd)  {
                        editmac(mdata, cnt);
                        break;
                }
}

void  delmac_clicked(struct macupddata *mdata)
{
        GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(mdata->view));
        GtkTreeIter  iter;
        if  (gtk_tree_selection_get_selected(sel, NULL, &iter))  {
                gint  seq;
                gtk_tree_model_get(GTK_TREE_MODEL(mdata->store), &iter, 0, &seq, -1);
                delete_macro(mdata->jorv, mdata->mlist, seq+1);
                gtk_list_store_remove(mdata->store, &iter);
                free(mdata->mlist[seq].cmd);
                free(mdata->mlist[seq].descr);
                mdata->mlist[seq].cmd = 0;
                mdata->mlist[seq].descr = 0;
                Dirty = 1;
        }
}

void  updmac_clicked(struct macupddata *mdata)
{
        GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(mdata->view));
        GtkTreeIter  iter;
        if  (gtk_tree_selection_get_selected(sel, NULL, &iter))  {
                gint  seq;
                gtk_tree_model_get(GTK_TREE_MODEL(mdata->store), &iter, 0, &seq, -1);
                editmac(mdata, seq);
        }
}

static void mlist_dblclk(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *col, struct macupddata *mdata)
{
        GtkTreeIter     iter;
        if  (gtk_tree_model_get_iter(GTK_TREE_MODEL(mdata->store), &iter, path))  {
                gint  seq;
                gtk_tree_model_get(GTK_TREE_MODEL(mdata->store), &iter, 0, &seq, -1);
                editmac(mdata, seq);
        }
}

void  macro_edit(const char jorv, struct macromenitem *mlist)
{
        GtkWidget  *dlg, *mwid, *scroll, *hbox, *butt;
        GtkCellRenderer     *rend;
        GtkListStore    *mlist_store;
        GtkTreeSelection *sel;
        struct  macupddata      mdata;
        int     cnt;
        char    *pr;

        mlist_store = gtk_list_store_new(3, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING);

        for  (cnt = 0;  cnt < MAXMACS;  cnt++)
                if  (mlist[cnt].cmd)  {
                        GtkTreeIter   iter;
                        gtk_list_store_append(mlist_store, &iter);
                        gtk_list_store_set(mlist_store, &iter, 0, cnt, 1, mlist[cnt].cmd, 2, mlist[cnt].descr, -1);
                }

        pr = gprompt($P{xbtq macedit dlg});
        dlg = gtk_dialog_new_with_buttons(pr, GTK_WINDOW(toplevel), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
        free(pr);
        mwid = gtk_tree_view_new();
        rend = gtk_cell_renderer_text_new();
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(mwid), -1, "Command", rend, "text", 1, NULL);
        rend = gtk_cell_renderer_text_new();
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(mwid), -1, "Description", rend, "text", 2, NULL);

        gtk_tree_view_set_model(GTK_TREE_VIEW(mwid), GTK_TREE_MODEL(mlist_store));
        g_object_unref(mlist_store);            /* So that it gets deallocated */

        scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_set_border_width(GTK_CONTAINER(scroll), 5);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(scroll), mwid);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), scroll, FALSE, FALSE, 0);

        mdata.view = mwid;
        mdata.store = mlist_store;
        mdata.mlist = mlist;
        mdata.jorv = jorv;

        hbox = gtk_hbox_new(TRUE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        butt = make_push_button($P{xbtq new macro});
        g_signal_connect_swapped(G_OBJECT(butt), "clicked", G_CALLBACK(newmac_clicked), &mdata);
        gtk_box_pack_start(GTK_BOX(hbox), butt, FALSE, FALSE, DEF_DLG_HPAD);
        butt = make_push_button($P{xbtq del macro});
        g_signal_connect_swapped(G_OBJECT(butt), "clicked", G_CALLBACK(delmac_clicked), &mdata);
        gtk_box_pack_start(GTK_BOX(hbox), butt, FALSE, FALSE, DEF_DLG_HPAD);
        butt = make_push_button($P{xbtq edit macro});
        g_signal_connect_swapped(G_OBJECT(butt), "clicked", G_CALLBACK(updmac_clicked), &mdata);
        g_signal_connect(G_OBJECT(mwid), "row-activated", (GCallback) mlist_dblclk, (gpointer) &mdata);
        gtk_box_pack_start(GTK_BOX(hbox), butt, FALSE, FALSE, DEF_DLG_HPAD);

        sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(mwid));
        gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);

        gtk_widget_show_all(dlg);
        gtk_dialog_run(GTK_DIALOG(dlg));
        gtk_widget_destroy(dlg);
}

void  cb_jmacedit()
{
        macro_edit('j', jobmacs);
}

void  cb_vmacedit()
{
        macro_edit('v', varmacs);
}
