/* xbq_vcall.c -- variable handling for gbch-xq

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
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <gtk/gtk.h>
#include <errno.h>
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
#include "gtk_lib.h"
#include "stringvec.h"

int             Const_val;              /* Value for constant in arith */

HelpaltRef      varexport_types;

extern  char    *execprog;
extern  char    *Curr_pwd;      /* Directory on entry */

struct  macromenitem    varmacs[MAXMACS];

/* Send var-type message to scheduler */

void  qwvmsg(unsigned code, BtvarRef vp, ULONG Sseq)
{
        Oreq.sh_params.mcode = code;
        if  (vp)
                Oreq.sh_un.sh_var = *vp;
        Oreq.sh_un.sh_var.var_sequence = Sseq;
        if  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(Btvar), 0) < 0)
                msg_error();
}

/* Display var-type error message */

void  qdoverror(unsigned retc, BtvarRef vp)
{
        switch  (retc  & REQ_TYPE)  {
        default:
                disp_arg[0] = retc;
                doerror($EH{Unexpected sched message});
                return;
        case  VAR_REPLY:
                disp_str = vp->var_name;
                doerror((int) ((retc & ~REQ_TYPE) + $EH{Base for scheduler var errors}));
                return;
        case  NET_REPLY:
                disp_str = vp->var_name;
                doerror((int) ((retc & ~REQ_TYPE) + $EH{Base for scheduler net errors}));
                return;
        }
}

GtkWidget *start_vardlg(BtvarRef vp, const int dlgcode, const int labelcode)
{
        GtkWidget  *dlg, *lab;
        char    *pr;
        GString  *labp;

        pr = gprompt(dlgcode);
        dlg = gtk_dialog_new_with_buttons(pr,
                                          GTK_WINDOW(toplevel),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK,
                                          GTK_RESPONSE_OK,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL,
                                          NULL);
        free(pr);

        pr = gprompt(labelcode);
        labp = g_string_new(pr);
        free(pr);
        g_string_append_c(labp, ' ');
        g_string_append(labp, VAR_NAME(vp));
        lab = gtk_label_new(labp->str);
        g_string_free(labp, TRUE);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), lab, FALSE, FALSE, DEF_DLG_VPAD);
        return  dlg;
}

static GtkWidget *make_var_export_combo()
{
        GtkWidget  *result = gtk_combo_box_new_text();
        int     cnt;

        for  (cnt = 0;  cnt < varexport_types->numalt;  cnt++)
                gtk_combo_box_append_text(GTK_COMBO_BOX(result), varexport_types->list[cnt]);
        return  result;
}

static int  val_var_name(const gchar *name)
{
        const  gchar  *cp = name;
        if  (!isalpha(*cp)  &&  *cp != '_')  {

                return  0;
        }
        while  (*++cp)  {
                if  (!isalnum(*cp)  &&  *cp != '_')  {
                        doerror($EH{xmbtq invalid var name});
                        return  0;
                }
        }
        if  (name - cp  > BTV_NAME)  {
                disp_arg[0] = BTV_NAME;
                doerror($EH{xmbtq varname too long});
                return  0;
        }
        return  1;
}

static int  val_var_num(const gchar *num)
{
        const  gchar  *cp = num;

        if  (*cp == '-')
                cp++;
        do  {
                if  (!isdigit(*cp))  {
                        doerror($EH{xbtq invalid number});
                        return  0;
                }
                cp++;
        }  while  (*cp);
        return  1;
}

void  cb_createv()
{
        GtkWidget  *dlg, *hbox, *vnamew, *commentw, *vtypew, *vvalt, *exportw;
        char    *pr;

        if  (!(mypriv->btu_priv & BTM_CREATE))  {
                doerror($EH{xmbtq no var create perm});
                return;
        }

        pr = gprompt($P{xbtq create var dlgtit});
        dlg = gtk_dialog_new_with_buttons(pr,
                                          GTK_WINDOW(toplevel),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK,
                                          GTK_RESPONSE_OK,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL,
                                          NULL);
        free(pr);

        /* First row - name */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq createv name lab}), FALSE, FALSE, DEF_DLG_HPAD);
        vnamew = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), vnamew, FALSE, FALSE, DEF_DLG_HPAD);

        /* Second row - comment */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq createv comment lab}), FALSE, FALSE, DEF_DLG_HPAD);
        commentw = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), commentw, FALSE, FALSE, DEF_DLG_HPAD);

        /* Third row value */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq createv value lab}), FALSE, FALSE, DEF_DLG_HPAD);
        vvalt = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), vvalt, FALSE, FALSE, DEF_DLG_HPAD);
        vtypew = gprompt_checkbutton($P{xbtq createv istext});
        gtk_box_pack_start(GTK_BOX(hbox), vtypew, FALSE, FALSE, DEF_DLG_HPAD);

        /* Fourth row export type */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq createv export lab}), FALSE, FALSE, DEF_DLG_HPAD);
        exportw = make_var_export_combo();
        gtk_box_pack_start(GTK_BOX(hbox), exportw, FALSE, FALSE, DEF_DLG_HPAD);
        gtk_combo_box_set_active(GTK_COMBO_BOX(exportw), 0);

        gtk_widget_show_all(dlg);
        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const  gchar  *vname = gtk_entry_get_text(GTK_ENTRY(vnamew));
                const  gchar  *txt = gtk_entry_get_text(GTK_ENTRY(vvalt));
                const  gchar  *comm = gtk_entry_get_text(GTK_ENTRY(commentw));
                gboolean  ist = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(vtypew));
                int     expt = gtk_combo_box_get_active(GTK_COMBO_BOX(exportw));
                unsigned  retc;

                if  (!val_var_name(vname))
                        continue;
                if  (!ist  &&  !val_var_num(txt))
                        continue;

                strcpy(Oreq.sh_un.sh_var.var_name, vname);
                strncpy(Oreq.sh_un.sh_var.var_comment, comm, BTV_COMMENT);
                if  (ist)  {
                        Oreq.sh_un.sh_var.var_value.const_type = CON_STRING;
                        strncpy(Oreq.sh_un.sh_var.var_value.con_un.con_string, txt, BTC_VALUE);
                }
                else  {
                        Oreq.sh_un.sh_var.var_value.const_type = CON_LONG;
                        Oreq.sh_un.sh_var.var_value.con_un.con_long = atol(txt);
                }
                Oreq.sh_un.sh_var.var_flags = 0;
                if  (expt > 0)  {
                        Oreq.sh_un.sh_var.var_flags |= VF_EXPORT;
                        if  (expt > 1)
                                Oreq.sh_un.sh_var.var_flags |= VF_CLUSTER;
                }
                Oreq.sh_un.sh_var.var_mode.u_flags = mypriv->btu_vflags[0];
                Oreq.sh_un.sh_var.var_mode.g_flags = mypriv->btu_vflags[1];
                Oreq.sh_un.sh_var.var_mode.o_flags = mypriv->btu_vflags[2];
                Oreq.sh_un.sh_var.var_type = 0;
                qwvmsg(V_CREATE, (BtvarRef) 0, 0L);
                if  ((retc = readreply()) != V_OK)
                        qdoverror(retc, &Oreq.sh_un.sh_var);
                break;
        }
        gtk_widget_destroy(dlg);
}

void  cb_renamev()
{
        GtkWidget  *dlg, *hbox, *newnw;
        BtvarRef  cv = getselectedvar(BTM_DELETE);
        ULONG   Saveseq;

        if  (!cv)
                return;

        if  (cv->var_id.hostid != 0)  {
                doerror($EH{xmbtq renaming remote var});
                return;
        }

        Saveseq = cv->var_sequence;

        dlg = start_vardlg(cv, $P{xbtq rename var dlgtit}, $P{xbtq rename var title});
        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq rename var newname lab}), FALSE, FALSE, DEF_DLG_HPAD);
        newnw = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), newnw, FALSE, FALSE, DEF_DLG_HPAD);
        gtk_entry_set_text(GTK_ENTRY(newnw), cv->var_name);
        gtk_widget_show_all(dlg);
        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const  gchar  *newname = gtk_entry_get_text(GTK_ENTRY(newnw));
                unsigned        retc;

                if  (val_var_name(newname))  {
                        if  (strcmp(newname, cv->var_name) != 0)  {
                                Oreq.sh_params.mcode = V_NEWNAME;
                                Oreq.sh_un.sh_rn.sh_ovar = *cv;
                                Oreq.sh_un.sh_rn.sh_ovar.var_sequence = Saveseq;
                                strcpy(Oreq.sh_un.sh_rn.sh_rnewname, newname);
                                if  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(Btvar) + strlen(newname) + 1, 0) < 0)
                                        msg_error();
                                if  ((retc = readreply()) != V_OK)
                                        qdoverror(retc, &Oreq.sh_un.sh_rn.sh_ovar);
                        }
                        break;
                }
        }
        gtk_widget_destroy(dlg);
}

void  cb_vexport()
{
        GtkWidget  *dlg, *hbox, *exportw;
        BtvarRef        cv = getselectedvar(BTM_DELETE);
        ULONG   Saveseq;
        int     wotv = 0;

        if  (!cv)
                return;

        if  (cv->var_id.hostid != 0)  {
                doerror($EH{xmbtq exporting remote var});
                return;
        }

        Saveseq = cv->var_sequence;

        dlg = start_vardlg(cv, $P{xbtq export var dlgtit}, $P{xbtq export var title});
        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq export var opt lab}), FALSE, FALSE, DEF_DLG_HPAD);
        exportw = make_var_export_combo();
        gtk_box_pack_start(GTK_BOX(hbox), exportw, FALSE, FALSE, DEF_DLG_HPAD);
        if  (cv->var_flags & VF_EXPORT)  {
                wotv++;
                if  (cv->var_flags & VF_CLUSTER)
                        wotv++;
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(exportw), wotv);
        gtk_widget_show_all(dlg);
        if  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                int     newv = gtk_combo_box_get_active(GTK_COMBO_BOX(exportw));
                if  (newv >= 0  &&  newv != wotv)  {
                        unsigned        retc;

                        Oreq.sh_un.sh_var = *cv;
                        Oreq.sh_un.sh_var.var_flags &= ~(VF_EXPORT|VF_CLUSTER);
                        if  (newv > 0)  {
                                Oreq.sh_un.sh_var.var_flags |= VF_EXPORT;
                                if  (newv > 1)
                                        Oreq.sh_un.sh_var.var_flags |= VF_CLUSTER;
                        }
                        qwvmsg(V_CHFLAGS, (BtvarRef) 0, Saveseq);
                        if  ((retc = readreply()) != V_OK)
                                qdoverror(retc, &Oreq.sh_un.sh_var);
                }
        }
        gtk_widget_destroy(dlg);
}

void  cb_vdel()
{
        BtvarRef        cv = getselectedvar(BTM_DELETE);
        unsigned        retc;
        ULONG           Saveseq;

        if  (!cv)
                return;
        Saveseq = cv->var_sequence;

        if  (Dispflags & DF_CONFABORT  &&  !Confirm($PH{xmbtq confirm delete var}))
                return;

        qwvmsg(V_DELETE, cv, Saveseq);
        if  ((retc = readreply()) != V_OK)
                qdoverror(retc, &Oreq.sh_un.sh_var);
}

void  cb_assign()
{
        GtkWidget  *dlg, *hbox, *vtypew, *vvalt;
        BtvarRef  cv = getselectedvar(BTM_READ|BTM_WRITE);
        ULONG           Saveseq;

        if  (!cv)
                return;

        Saveseq = cv->var_sequence;

        dlg = start_vardlg(cv, $P{xbtq assign var dlgtit}, $P{xbtq assign var title});
        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq assign var value lab}), FALSE, FALSE, DEF_DLG_HPAD);
        vvalt = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), vvalt, FALSE, FALSE, DEF_DLG_HPAD);
        vtypew = gprompt_checkbutton($P{xbtq assign istext});
        gtk_box_pack_start(GTK_BOX(hbox), vtypew, FALSE, FALSE, DEF_DLG_HPAD);
        if  (cv->var_value.const_type == CON_STRING)  {
                gtk_entry_set_text(GTK_ENTRY(vvalt), cv->var_value.con_un.con_string);
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(vtypew), TRUE);
        }
        else  {
                char            nbuf[20];
                sprintf(nbuf, "%ld", (long) cv->var_value.con_un.con_long);
                gtk_entry_set_text(GTK_ENTRY(vvalt), nbuf);
        }

        gtk_widget_show_all(dlg);

        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const  gchar  *newval = gtk_entry_get_text(GTK_ENTRY(vvalt));
                gboolean        ist = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(vtypew));
                unsigned        retc;

                if  (!ist  &&  !val_var_num(newval))
                        continue;

                Oreq.sh_un.sh_var = *cv;

                if  (ist)  {
                        Oreq.sh_un.sh_var.var_value.const_type = CON_STRING;
                        strncpy(Oreq.sh_un.sh_var.var_value.con_un.con_string, newval, BTC_VALUE);
                }
                else  {
                        Oreq.sh_un.sh_var.var_value.const_type = CON_LONG;
                        Oreq.sh_un.sh_var.var_value.con_un.con_long = atol(newval);
                }

                qwvmsg(V_ASSIGN, (BtvarRef) 0, Saveseq);

                if  ((retc = readreply()) != V_OK)
                        qdoverror(retc, &Oreq.sh_un.sh_var);

                break;
        }
        gtk_widget_destroy(dlg);
}

void  cb_vcomment()
{
        GtkWidget  *dlg, *hbox, *commentw;
        BtvarRef  cv = getselectedvar(BTM_READ|BTM_WRITE);
        ULONG           Saveseq;

        if  (!cv)
                return;

        Saveseq = cv->var_sequence;

        dlg = start_vardlg(cv, $P{xbtq comment var dlgtit}, $P{xbtq comment var title});
        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq set comment lab}), FALSE, FALSE, DEF_DLG_HPAD);
        commentw = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), commentw, FALSE, FALSE, DEF_DLG_HPAD);
        gtk_entry_set_text(GTK_ENTRY(commentw), cv->var_comment);
        gtk_widget_show_all(dlg);

        if  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const  gchar  *newval = gtk_entry_get_text(GTK_ENTRY(commentw));
                if  (strcmp(newval, cv->var_comment) != 0)  {
                        unsigned        retc;
                        Oreq.sh_un.sh_var = *cv;
                        strncpy(Oreq.sh_un.sh_var.var_comment, newval, BTV_COMMENT);
                        qwvmsg(V_CHCOMM, (BtvarRef) 0, Saveseq);
                        if  ((retc = readreply()) != V_OK)
                                qdoverror(retc, &Oreq.sh_un.sh_var);
                }
        }

        gtk_widget_destroy(dlg);
}

void  cb_cassign()
{
        GtkWidget  *dlg, *hbox, *conw;
        GtkAdjustment   *adj;

        dlg = gprompt_dialog(toplevel, $P{xbtq set const val dlgtit});
        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq set const lab}), FALSE, FALSE, DEF_DLG_HPAD);
        adj = (GtkAdjustment *) gtk_adjustment_new((gdouble) Const_val, -99999999.0, 99999999.0, 1.0, 10.0, 0.0);
        conw = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), conw, FALSE, FALSE, DEF_DLG_HPAD);
        gtk_widget_show_all(dlg);

        if  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)
                Const_val = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(conw));

        gtk_widget_destroy(dlg);
}

void  cb_arith(GtkAction *action)
{
        BtvarRef        cv = getselectedvar(BTM_READ|BTM_WRITE);
        BtconRef        cvalue;
        LONG            newvalue;
        ULONG           Saveseq;
        unsigned        retc;

        if  (!cv)
                return;

        Saveseq = cv->var_sequence;
        cvalue = &cv->var_value;
        if  (cvalue->const_type != CON_LONG)  {
                doerror($EH{xmbtq value not arith});
                return;
        }
        newvalue = cvalue->con_un.con_long;

        switch  (gtk_action_get_name(action)[0])  {
        default:
                return;
        case  'P':
                newvalue += Const_val;
                break;

        case  'M':
                if  (strcmp(gtk_action_get_name(action), "Minus") == 0)  {
                        newvalue -= Const_val;
                        break;
                }
                if  (Const_val == 0L)  {
                        doerror($EH{xmbtq divide by zero});
                        return;
                }
                newvalue %= Const_val;
                break;

        case  'T':
                newvalue *= Const_val;
                break;

        case  'D':
                if  (Const_val == 0L)  {
                        doerror($EH{xmbtq divide by zero});
                        return;
                }
                newvalue /= Const_val;
        }

        Oreq.sh_un.sh_var = *cv;
        Oreq.sh_un.sh_var.var_value.con_un.con_long = newvalue;
        qwvmsg(V_ASSIGN, (BtvarRef) 0, Saveseq);
        if  ((retc = readreply()) != V_OK)
                qdoverror(retc, &Oreq.sh_un.sh_var);
}

extern  void    setup_vmodebits(GtkWidget *, GtkWidget *[3][NUM_VMODEBITS], USHORT, USHORT, USHORT);
extern  void    read_vmodes(GtkWidget *[3][NUM_VMODEBITS], USHORT *, USHORT *, USHORT *);

void  cb_vperm()
{
        BtvarRef  cv = getselectedvar(BTM_WRMODE);
        GtkWidget       *dlg, *frame, *vmodes[3][NUM_VMODEBITS];
        char    *pr;
        ULONG   Saveseq;
        Btvar   cvar;

        if  (!cv)
                return;

        Saveseq = cv->var_sequence;
        cvar = *cv;

        dlg = start_vardlg(&cvar, $P{xbtq var mode dlgtit}, $P{xbtq var mode lab});
        pr = gprompt($P{xbtq var mode frame lab});
        frame = gtk_frame_new(pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);
        setup_vmodebits(frame, vmodes, cvar.var_mode.u_flags, cvar.var_mode.g_flags, cvar.var_mode.o_flags);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame, TRUE, TRUE, DEF_DLG_VPAD);
        gtk_widget_show_all(dlg);

        if  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                read_vmodes(vmodes, &cvar.var_mode.u_flags, &cvar.var_mode.g_flags, &cvar.var_mode.o_flags);
                if  (cvar.var_mode.u_flags != cv->var_mode.u_flags ||
                     cvar.var_mode.g_flags != cv->var_mode.g_flags ||
                     cvar.var_mode.o_flags != cv->var_mode.o_flags)  {
                        unsigned  retc;
                        Oreq.sh_un.sh_var = cvar;
                        qwvmsg(V_CHMOD, (BtvarRef) 0, Saveseq);
                        if  ((retc = readreply()) != V_OK)
                                qdoverror(retc, &Oreq.sh_un.sh_var);
                }
        }
        gtk_widget_destroy(dlg);
}

void  cb_vowner()
{
        BtvarRef  cv = getselectedvar(BTM_RDMODE);
        GtkWidget  *dlg, *hbox, *usel, *gsel;
        ULONG   Saveseq;
        int     uw, cnt;
        char    **uglist, **up;

        if  (!cv)
                return;

        Saveseq = cv->var_sequence;

        dlg = start_vardlg(cv, $P{xbtq var owner dlgtit}, $P{xbtq var owner lab});
        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq var owner user}), FALSE, FALSE, DEF_DLG_HPAD);
        usel = gtk_combo_box_new_text();
        gtk_box_pack_start(GTK_BOX(hbox), usel, FALSE, FALSE, DEF_DLG_VPAD);
        uw = -1;
        cnt = 0;
        uglist = gen_ulist((char *) 0);

        for  (up = uglist;  *up;  up++)  {
                gtk_combo_box_append_text(GTK_COMBO_BOX(usel), *up);
                if  (strcmp(*up, cv->var_mode.o_user) == 0)
                        uw = cnt;
                cnt++;
        }
        freehelp(uglist);
        if  (uw >= 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(usel), uw);

        /* Same for groups */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq var owner group}), FALSE, FALSE, DEF_DLG_HPAD);
        gsel = gtk_combo_box_new_text();
        gtk_box_pack_start(GTK_BOX(hbox), gsel, FALSE, FALSE, DEF_DLG_VPAD);
        uw = -1;
        cnt = 0;
        uglist = gen_glist((char *) 0);

        for  (up = uglist;  *up;  up++)  {
                gtk_combo_box_append_text(GTK_COMBO_BOX(gsel), *up);
                if  (strcmp(*up, cv->var_mode.o_group) == 0)
                        uw = cnt;
                cnt++;
        }
        freehelp(uglist);
        if  (uw >= 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(gsel), uw);

        if  (!(mypriv->btu_priv & BTM_WADMIN))  {
                if  (!mpermitted(&cv->var_mode, cv->var_mode.o_uid == Realuid? BTM_UGIVE: BTM_UTAKE, mypriv->btu_priv))
                        gtk_widget_set_sensitive(usel, FALSE);
                if  (!mpermitted(&cv->var_mode, cv->var_mode.o_gid == Realgid? BTM_GGIVE: BTM_GTAKE, mypriv->btu_priv))
                        gtk_widget_set_sensitive(gsel, FALSE);
        }

        gtk_widget_show_all(dlg);

        if  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                gchar  *newug;

                newug = gtk_combo_box_get_active_text(GTK_COMBO_BOX(gsel));
                if  (strcmp(newug, cv->var_mode.o_group) != 0)  {
                        int_ugid_t  nug = lookup_gname(newug);
                        unsigned        retc;

                        if  (nug == UNKNOWN_GID)
                                doerror($EH{xmbtq invalid group});
                        else  {
                                Oreq.sh_params.param = nug;
                                qwvmsg(V_CHGRP, cv, Saveseq);
                                if  ((retc = readreply()) != V_OK)
                                        qdoverror(retc, &Oreq.sh_un.sh_var);
                                Saveseq++;
                        }
                }
                g_free(newug);
                newug = gtk_combo_box_get_active_text(GTK_COMBO_BOX(usel));
                if  (strcmp(newug, cv->var_mode.o_user) != 0)  {
                        int_ugid_t      nug = lookup_uname(newug);
                        unsigned        retc;

                        if  (nug == UNKNOWN_UID)
                                doerror($EH{xmbtq invalid user});
                        else  {
                                Oreq.sh_params.param = nug;
                                qwvmsg(V_CHOWN, cv, Saveseq);
                                if  ((retc = readreply()) != V_OK)
                                        qdoverror(retc, &Oreq.sh_un.sh_var);
                        }
                }
                g_free(newug);
        }

        gtk_widget_destroy(dlg);
}

/*
 ********************************************************************************
 *
 *                                VAR MACROS
 *
 ********************************************************************************
 */

static int  vmacroexec(const char *str, BtvarRef vp)
{
        PIDTYPE pid;
        int     status;

        if  (!execprog)
                execprog = envprocess(EXECPROG);

        if  ((pid = fork()) == 0)  {
                const  char     *argbuf[3];
                argbuf[0] = str;
                if  (vp)  {
                        argbuf[1] = VAR_NAME(vp);
                        argbuf[2] = (const char *) 0;
                }
                else
                        argbuf[1] = (const char *) 0;
                Ignored_error = chdir(Curr_pwd);
                execv(execprog, (char **) argbuf);
                exit(E_BTEXEC1);
        }
        if  (pid < 0)  {
                doerror($EH{xmbtq var macro cannot fork});
                return  0;
        }
#ifdef  HAVE_WAITPID
        while  (waitpid(pid, &status, 0) < 0)
                ;
#else
        while  (wait(&status) != pid)
                ;
#endif
        if  (status != 0)  {
                if  (status & 255)  {
                        disp_arg[0] = status & 255;
                        doerror($EH{xmbtq var macro signal});
                }
                else  {
                        disp_arg[0] = (status >> 8) & 255;
                        doerror($EH{xmbtq var macro exit code});
                }
                return  0;
        }

        return  1;
}

static  struct  stringvec  previous_commands;

/* Version of var macro for where we prompt */

void  cb_vmac()
{
        BtvarRef  vp = getselectedvar(0);
        GtkWidget  *dlg, *lab, *hbox, *cmdentry;
        int        oldmac = is_init(previous_commands);
        char       *pr;

        dlg = gprompt_dialog(toplevel, $P{xbtq vmac dlg});

        if  (vp)  {
                GString *labp = g_string_new(NULL);
                pr = gprompt($P{xbtq vmac named});
                g_string_printf(labp, "%s %s", pr, VAR_NAME(vp));
                free(pr);
                lab = gtk_label_new(labp->str);
                g_string_free(labp, TRUE);
        }
        else
                lab = gprompt_label($P{xbtq vmac noname});

        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), lab, FALSE, FALSE, 0);

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, 0);
        lab = gprompt_label($P{xbtq vmac cmd});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, 0);

        if  (oldmac)  {
                unsigned  cnt;
                cmdentry = gtk_combo_box_entry_new_text();
                for  (cnt = 0;  cnt < stringvec_count(previous_commands);  cnt++)
                        gtk_combo_box_append_text(GTK_COMBO_BOX(cmdentry), stringvec_nth(previous_commands, cnt));
        }
        else
                cmdentry = gtk_entry_new();

        gtk_box_pack_start(GTK_BOX(hbox), cmdentry, FALSE, FALSE, 0);
        gtk_widget_show_all(dlg);
        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const  char  *cmdtext;
                if  (oldmac)
                        cmdtext = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(cmdentry))));
                else
                        cmdtext = gtk_entry_get_text(GTK_ENTRY(cmdentry));
                if  (strlen(cmdtext) == 0)  {
                        doerror($EH{xbtq empty macro command});
                        continue;
                }

                if  (vmacroexec(cmdtext, vp))  {
                        if  (add_macro_to_list(cmdtext, 'v', varmacs))
                                break;
                        if  (!oldmac)
                                stringvec_init(&previous_commands);
                        stringvec_insert_unique(&previous_commands, cmdtext);
                        break;
                }
        }
        gtk_widget_destroy(dlg);
}

void  vmacruncb(GtkAction *act, struct macromenitem *mitem)
{
        vmacroexec(mitem->cmd, getselectedvar(0));
}
