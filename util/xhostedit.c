/* xhostedit.c -- main module for host file editing

   Copyright 2010 Free Software Foundation, Inc.

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

#include <stdio.h>
#include <gtk/gtk.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <pwd.h>
#include <ctype.h>
#include "config.h"
#include "defaults.h"
#include "incl_unix.h"
#include "networkincl.h"
#include "remote.h"
#include "hostedit.h"

static	char	rcsid2[] = "@(#) $Revision: 1.1 $";

#define	DEF_PAD	10
#define	DEF_PAD_SMALL	5

#define CLIENT_COL	0
#define HOST_COL	1
#define ALIAS_COL	2
#define IP_COL		3
#define DEF_COL		4
#define	PROBE_COL	5
#define MANUAL_COL	6
#define	PWCHK_COL	7
#define	TRUST_COL	8
#define TIMEOUT_COL	9

GtkWidget	*toplevel, *hwid, *locwid, *defuwid;
GtkListStore	*hlist_store;

char	*authlist[] =  { "John M Collins", NULL  };

void	doerror(GtkWidget *parent, const char *msg, const char *arg)
{
	GtkWidget  *msgw;
	msgw = gtk_message_dialog_new(GTK_WINDOW(parent), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, msg, arg);
	g_signal_connect_swapped(msgw, "response", G_CALLBACK(gtk_widget_destroy), msgw);
	gtk_widget_show(msgw);
	gtk_dialog_run(GTK_DIALOG(msgw));
}

int	confirm(const char *msg)
{
	GtkWidget  *dlg = gtk_message_dialog_new(GTK_WINDOW(toplevel), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "%s", msg);
	int  ret = gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_YES;
	gtk_widget_destroy(dlg);
	return  ret;
}

int	validuser(const gchar *u)
{
	if  (!getpwnam(u))  {
		doerror(toplevel, "%s is not a valid user", u);
		return  0;
	}
	return  1;
}

enum  IPatype  validhostname(const char *h, netid_t *resip)
{
	if  (isdigit(h[0]))  {
		netid_t  res;
#ifdef	DGAVIION
		struct	in_addr	ina_str;
		ina_str = inet_addr(h);
		res = ina_str.s_addr;
#else
		res = inet_addr(h);
#endif
		if  (res == -1L)  {
			doerror(toplevel, "%s is not a valid IP address", h);
			return  NO_IPADDR;
		}
		*resip = res;
		return  IPADDR_IP;
	}
	else  {
		struct  hostent  *hp = gethostbyname(h);
		if  (!hp)  {
			doerror(toplevel, "%s is not a valid host name", h);
			return  NO_IPADDR;
		}
		*resip = *(netid_t *) hp->h_addr;
		return  IPADDR_NAME;
	}
}

static void cb_about()
{
	GtkWidget  *dlg = gtk_about_dialog_new();
	char	*cp = strchr(rcsid2, ':');
	char	vbuf[20];

	if  (!cp)
		strcpy(vbuf, "Initial version");
	else  {
		char  *ep;
		cp++;
		ep = strchr(cp, '$');
		int  n = ep - cp;
		strncpy(vbuf, cp, n);
		vbuf[n] = '\0';
	}
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dlg), vbuf);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dlg), "Xi Software Ltd 2010");
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dlg), "http://www.xisl.com");
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dlg), (const char **) authlist);
	gtk_dialog_run(GTK_DIALOG(dlg));
	gtk_widget_destroy(dlg);
}

void	upd_hrow(const struct remote *hp, GtkTreeIter *iter)
{
	if  (hp->ht_flags & HT_DOS)  {
		gtk_list_store_set(hlist_store, iter, CLIENT_COL, TRUE,
						      PWCHK_COL, hp->ht_flags & HT_PWCHECK? TRUE: FALSE,
						      -1);
		if  (hp->ht_flags & HT_ROAMUSER)
			gtk_list_store_set(hlist_store, iter, HOST_COL, hp->hostname, ALIAS_COL, hp->alias, DEF_COL, hp->dosuser, -1);
		else  if  (hp->ht_flags & HT_HOSTISIP)
			gtk_list_store_set(hlist_store, iter,
					   HOST_COL, phname(hp->hostid, IPADDR_IP),
					   ALIAS_COL, hp->hostname,
					   DEF_COL, hp->dosuser, -1);
		else
			gtk_list_store_set(hlist_store, iter,
					   HOST_COL, hp->hostname,
					   ALIAS_COL, hp->alias,
					   IP_COL, phname(hp->hostid, IPADDR_IP),
					   DEF_COL, hp->dosuser, -1);
	}
	else  {
		const  char  *h, *a, *i;

		if  (hp->ht_flags & HT_HOSTISIP)  {
			h = phname(hp->hostid, IPADDR_IP);
			a = hp->hostname;
			i = "";
		}
		else  {
			h = hp->hostname;
			a = hp->alias;
			i = phname(hp->hostid, IPADDR_IP);
		}
		gtk_list_store_set(hlist_store, iter,
				   CLIENT_COL, FALSE,
				   HOST_COL, h,
				   ALIAS_COL, a,
				   IP_COL, i,
				   PROBE_COL, hp->ht_flags & HT_PROBEFIRST? TRUE: FALSE,
				   MANUAL_COL, hp->ht_flags & HT_MANUAL? TRUE: FALSE,
				   TRUST_COL, hp->ht_flags & HT_TRUSTED? TRUE: FALSE,
				   -1);
	}
	gtk_list_store_set(hlist_store, iter, TIMEOUT_COL, hp->ht_timeout, -1);
}

GtkWidget *make_timeout_w(GtkWidget *vbox)
{
	GtkWidget  *hbox, *spinb, *lab;
	GtkAdjustment	*adj;

	hbox = gtk_hbox_new(FALSE, DEF_PAD);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_PAD);
	lab = gtk_label_new("Timeout");
	gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_PAD);
	adj = (GtkAdjustment *) gtk_adjustment_new((gdouble) NETTICKLE, 0.0, 65535.0, 100.0, 1000.0, 0.0);
	spinb = gtk_spin_button_new(adj, 1.0, 0);
	gtk_box_pack_start(GTK_BOX(hbox), spinb, FALSE, FALSE, DEF_PAD);
	return  spinb;
}

struct	uhost_ddata  {
	GtkWidget  *hname, *alias, *probe, *trusted, *manual, *timo;
};

GtkWidget  *make_uhost_dlg(GtkWidget *parent, char *msg, struct uhost_ddata *ddata)
{
	GtkWidget  *dlg, *lab, *hbox;

	dlg = gtk_dialog_new_with_buttons(msg,
					  GTK_WINDOW(parent),
					  GTK_DIALOG_DESTROY_WITH_PARENT,
					  GTK_STOCK_OK,
					  GTK_RESPONSE_OK,
					  GTK_STOCK_CANCEL,
					  GTK_RESPONSE_CANCEL,
					  NULL);

	/* host name */

	hbox = gtk_hbox_new(FALSE, DEF_PAD);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_PAD);
	lab = gtk_label_new("Host name (or IP)");
	gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_PAD);
	ddata->hname = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), ddata->hname, FALSE, FALSE, DEF_PAD);

	/* alias */

	hbox = gtk_hbox_new(FALSE, DEF_PAD);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_PAD);
	lab = gtk_label_new("Alias");
	gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_PAD);
	ddata->alias = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), ddata->alias, FALSE, FALSE, DEF_PAD);

	ddata->probe = gtk_check_button_new_with_label("Probe first");
	ddata->manual = gtk_check_button_new_with_label("Manual connections");
	ddata->trusted = gtk_check_button_new_with_label("Trusted host");
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), ddata->probe, FALSE, FALSE, DEF_PAD);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), ddata->manual, FALSE, FALSE, DEF_PAD);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), ddata->trusted, FALSE, FALSE, DEF_PAD);
	ddata->timo = make_timeout_w(GTK_DIALOG(dlg)->vbox);
	gtk_widget_show_all(dlg);
	return  dlg;
}

int	extract_hostnames(struct remote *rp, GtkWidget *hwig, GtkWidget *awig)
{
	const gchar *h = gtk_entry_get_text(GTK_ENTRY(hwig));
	const gchar *a = gtk_entry_get_text(GTK_ENTRY(awig));
	netid_t  resip;
	enum IPatype  htype;

	if  (strlen(h) >= HOSTNSIZE)  {
		doerror(toplevel, "Sorry host name %s is too long", h);
		return  0;
	}
	if  (strlen(a) >= HOSTNSIZE)  {
		doerror(toplevel, "Sorry alias name %s is too long", a);
		return  0;
	}

	htype = validhostname(h, &resip);
	if  (htype == NO_IPADDR)
		return  0;

	if  (htype == IPADDR_IP)  {
		char	*c;
		if  (strlen(a) == 0)  {
			doerror(toplevel, "Alias must be given with numeric host %s", h);
			return  0;
		}
		if  (hnameclashes(a)  &&  strcmp(a, rp->alias) != 0)  {
			doerror(toplevel, "Alias name %s clashes with existing name", a);
			return  0;
		}
		c = ipclashes(resip);
		if  (c  &&  resip != rp->hostid)  {
			doerror(toplevel, "IP address clashes with existing IP for %s", c);
			return  0;
		}
		rp->hostid = resip;
		rp->ht_flags = HT_HOSTISIP;
	}
	else  {
		char	*c;
		if  (hnameclashes(h)  &&  strcmp(h, rp->hostname) != 0)  {
			doerror(toplevel, "Host name %s clashes with an existing name", h);
			return  0;
		}
		c = ipclashes(resip);
		if  (c  &&  resip != rp->hostid)  {
			doerror(toplevel, "IP address clashes with existing for %s", c);
			return  0;
		}
		if  (strlen(a) != 0  &&  hnameclashes(a)  &&  strcmp(a, rp->alias) != 0)  {
			doerror(toplevel, "Alias name %s clashes with an existing name", a);
			return  0;
		}
		rp->ht_flags = 0;
		rp->hostid = resip;
	}
	strncpy(rp->hostname, h, HOSTNSIZE-1);
	strncpy(rp->alias, a, HOSTNSIZE-1);
	return  1;
}

int	extract_uhost_dlg(struct remote *rp, struct uhost_ddata *ddata)
{
	if  (!extract_hostnames(rp, ddata->hname, ddata->alias))
		return  0;
	rp->dosuser[0] = '\0';
	if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->probe)))
		rp->ht_flags |= HT_PROBEFIRST;
	if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->manual)))
		rp->ht_flags |= HT_MANUAL;
	if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->trusted)))
		rp->ht_flags |= HT_TRUSTED;
	rp->ht_timeout = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ddata->timo));
	return  1;
}

struct	chost_ddata  {
	GtkWidget  *hname, *alias, *defu, *pwchk, *timo;
};

GtkWidget  *make_chost_dlg(GtkWidget *parent, char *msg, struct chost_ddata *ddata)
{
	GtkWidget  *dlg, *lab, *hbox;

	dlg = gtk_dialog_new_with_buttons(msg,
					  GTK_WINDOW(parent),
					  GTK_DIALOG_DESTROY_WITH_PARENT,
					  GTK_STOCK_OK,
					  GTK_RESPONSE_OK,
					  GTK_STOCK_CANCEL,
					  GTK_RESPONSE_CANCEL,
					  NULL);

	/* host name */

	hbox = gtk_hbox_new(FALSE, DEF_PAD);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_PAD);
	lab = gtk_label_new("Host name (or IP)");
	gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_PAD);
	ddata->hname = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), ddata->hname, FALSE, FALSE, DEF_PAD);

	/* alias */

	hbox = gtk_hbox_new(FALSE, DEF_PAD);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_PAD);
	lab = gtk_label_new("Alias");
	gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_PAD);
	ddata->alias = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), ddata->alias, FALSE, FALSE, DEF_PAD);

	/* default user */

	hbox = gtk_hbox_new(FALSE, DEF_PAD);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_PAD);
	lab = gtk_label_new("Default user");
	gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_PAD);
	ddata->defu = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), ddata->defu, FALSE, FALSE, DEF_PAD);

	ddata->pwchk = gtk_check_button_new_with_label("Password check");
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), ddata->pwchk, FALSE, FALSE, DEF_PAD);
	ddata->timo = make_timeout_w(GTK_DIALOG(dlg)->vbox);
	gtk_widget_show_all(dlg);
	return  dlg;
}

int	extract_chost_dlg(struct remote *rp, struct chost_ddata *ddata)
{
	const gchar *u;

	if  (!extract_hostnames(rp, ddata->hname, ddata->alias))
		return  0;

	u = gtk_entry_get_text(GTK_ENTRY(ddata->defu));
	if  (strlen(u) > UIDSIZE)  {
		doerror(toplevel, "Sorry user name %s is too long", u);
		return  0;
	}
	if  (!validuser(u))
		return  0;
	strncpy(rp->dosuser, u, UIDSIZE);
	rp->ht_flags |= HT_DOS;
	if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->pwchk)))
		rp->ht_flags |= HT_PWCHECK;
	rp->ht_timeout = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ddata->timo));
	return  1;
}

struct	cluhost_ddata  {
	GtkWidget  *unixname, *winname, *machname, *pwchk, *timo;
};

GtkWidget  *make_cluhost_dlg(GtkWidget *parent, char *msg, struct cluhost_ddata *ddata)
{
	GtkWidget  *dlg, *lab, *hbox;

	dlg = gtk_dialog_new_with_buttons(msg,
					  GTK_WINDOW(parent),
					  GTK_DIALOG_DESTROY_WITH_PARENT,
					  GTK_STOCK_OK,
					  GTK_RESPONSE_OK,
					  GTK_STOCK_CANCEL,
					  GTK_RESPONSE_CANCEL,
					  NULL);

	/* Unix user name */

	hbox = gtk_hbox_new(FALSE, DEF_PAD);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_PAD);
	lab = gtk_label_new("UNIX user name");
	gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_PAD);
	ddata->unixname = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), ddata->unixname, FALSE, FALSE, DEF_PAD);

	/* Windows name */

	hbox = gtk_hbox_new(FALSE, DEF_PAD);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_PAD);
	lab = gtk_label_new("Windows user name");
	gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_PAD);
	ddata->winname = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), ddata->winname, FALSE, FALSE, DEF_PAD);

	/* default machine */

	hbox = gtk_hbox_new(FALSE, DEF_PAD);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_PAD);
	lab = gtk_label_new("Default machine");
	gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_PAD);
	ddata->machname = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), ddata->machname, FALSE, FALSE, DEF_PAD);

	ddata->pwchk = gtk_check_button_new_with_label("Password check");
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), ddata->pwchk, FALSE, FALSE, DEF_PAD);
	ddata->timo = make_timeout_w(GTK_DIALOG(dlg)->vbox);
	gtk_widget_show_all(dlg);
	return  dlg;
}

int	extract_cluhost_dlg(struct remote *rp, struct cluhost_ddata *ddata)
{
	const gchar *uu = gtk_entry_get_text(GTK_ENTRY(ddata->unixname));
	const gchar *wu = gtk_entry_get_text(GTK_ENTRY(ddata->winname));
	const gchar *mch = gtk_entry_get_text(GTK_ENTRY(ddata->machname));

	if  (strlen(uu) >= HOSTNSIZE)  {
		doerror(toplevel, "Sorry user name %s is too long", uu);
		return  0;
	}
	if  (!validuser(uu))
		return  0;

	if  (strlen(wu) >= HOSTNSIZE)  {
		doerror(toplevel, "Sorry windows user name %s is too long", wu);
		return  0;
	}
	if  (strlen(mch) > 0)  {
		netid_t  resid = 0;
		if  (strlen(mch) >= sizeof(rp->dosuser))  {
			doerror(toplevel, "Sorry machine name %s is too long", mch);
			return  0;
		}
		if  (validhostname(mch, &resid) == NO_IPADDR)
			return  0;
	}

	strncpy(rp->hostname, uu, HOSTNSIZE-1);
	strncpy(rp->alias, wu, HOSTNSIZE-1);
	strncpy(rp->dosuser, mch, sizeof(rp->dosuser)-1);
	rp->ht_flags = HT_DOS|HT_ROAMUSER;
	if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->pwchk)))
		rp->ht_flags |= HT_PWCHECK;
	rp->ht_timeout = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ddata->timo));
	return  1;
}

void	lochdisplay()
{
	GString  *loch = g_string_new(NULL);
	if  (hadlocaddr == IPADDR_NAME)
		g_string_printf(loch, "%s ", phname(myhostid, IPADDR_NAME));
	else  if  (hadlocaddr == NO_IPADDR)
		g_string_printf(loch, "Current: %s ", phname(myhostid, IPADDR_NAME));
	g_string_append(loch, phname(myhostid, IPADDR_IP));
	gtk_entry_set_text(GTK_ENTRY(locwid), loch->str);
	g_string_free(loch, TRUE);
}

void	defudisplay()
{
	gtk_entry_set_text(GTK_ENTRY(defuwid), defcluser);
}

void	cb_addhost()
{
	struct  remote  dummr;
	struct  uhost_ddata  ddata;
	GtkWidget  *dlg;

	dlg = make_uhost_dlg(toplevel, "Create new Unix host", &ddata);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata.probe), TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata.trusted), TRUE);

	dummr.hostname[0] = '\0';
	dummr.alias[0] = '\0';
	dummr.hostid = 0;

	while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
		if  (extract_uhost_dlg(&dummr, &ddata))  {
			GtkTreeIter	iter;
			addhostentry(&dummr);
			gtk_list_store_append(hlist_store, &iter);
			upd_hrow(&hostlist[hostnum-1], &iter);
			break;
		}
	}
	gtk_widget_destroy(dlg);
}

void	cb_addclient()
{
	struct  remote  dummr;
	struct  chost_ddata  ddata;
	GtkWidget  *dlg;

	dlg = make_chost_dlg(toplevel, "Create new Client host", &ddata);

	dummr.hostname[0] = '\0';
	dummr.alias[0] = '\0';
	dummr.dosuser[0] = '\0';
	dummr.hostid = 0;

	while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
		if  (extract_chost_dlg(&dummr, &ddata))  {
			GtkTreeIter	iter;
			addhostentry(&dummr);
			gtk_list_store_append(hlist_store, &iter);
			upd_hrow(&hostlist[hostnum-1], &iter);
			break;
		}
	}
	gtk_widget_destroy(dlg);
}

void	cb_addclientuser()
{
	struct  remote  dummr;
	struct  cluhost_ddata  ddata;
	GtkWidget  *dlg;

	dlg = make_cluhost_dlg(toplevel, "Create new Client user record", &ddata);
	dummr.hostname[0] = '\0';
	dummr.alias[0] = '\0';
	dummr.dosuser[0] = '\0';
	dummr.hostid = 0;
	while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
		if  (extract_cluhost_dlg(&dummr, &ddata))  {
			GtkTreeIter	iter;
			addhostentry(&dummr);
			gtk_list_store_append(hlist_store, &iter);
			upd_hrow(&hostlist[hostnum-1], &iter);
			break;
		}
	}
	gtk_widget_destroy(dlg);
}

int	getselectedhost()
{
	GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(hwid));
	GtkTreeIter  iter;

	if  (gtk_tree_selection_get_selected(sel, NULL, &iter))  {
		GtkTreePath  *pth = gtk_tree_model_get_path(GTK_TREE_MODEL(hlist_store), &iter);
		gint	*ind = gtk_tree_path_get_indices(pth);
		int  ret = ind[0];
		gtk_tree_path_free(pth);
		return  ret;
	}
	doerror(toplevel, "No host name selected", (char *) 0);
	return  -1;
}

void	cb_edit()
{
	int	row = getselectedhost();
	struct  remote  *rp;
	GtkWidget  *dlg;
	GtkTreeIter  iter;

	if  (row < 0)
		return;
	rp = &hostlist[row];
	if  (rp->ht_flags & HT_DOS)  {
		if  (rp->ht_flags & HT_ROAMUSER)  {
			struct  cluhost_ddata  ddata;
			dlg = make_cluhost_dlg(toplevel, "Edit client user", &ddata);
			gtk_entry_set_text(GTK_ENTRY(ddata.unixname), rp->hostname);
			gtk_entry_set_text(GTK_ENTRY(ddata.winname), rp->alias);
			gtk_entry_set_text(GTK_ENTRY(ddata.machname), rp->dosuser);
			if  (rp->ht_flags & HT_PWCHECK)
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata.pwchk), TRUE);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(ddata.timo), (gdouble) rp->ht_timeout);
			while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK  &&  !extract_cluhost_dlg(rp, &ddata))
				;
		}
		else  {
			struct  chost_ddata  ddata;
			dlg = make_chost_dlg(toplevel, "Edit client host", &ddata);
			gtk_entry_set_text(GTK_ENTRY(ddata.hname), rp->hostname);
			gtk_entry_set_text(GTK_ENTRY(ddata.alias), rp->alias);
			gtk_entry_set_text(GTK_ENTRY(ddata.defu), rp->dosuser);
			if  (rp->ht_flags & HT_PWCHECK)
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata.pwchk), TRUE);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(ddata.timo), (gdouble) rp->ht_timeout);
			while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK  &&  !extract_chost_dlg(rp, &ddata))
				;
		}
	}
	else  {
		struct  uhost_ddata  ddata;
		dlg = make_uhost_dlg(toplevel, "Edit Unix host", &ddata);
		gtk_entry_set_text(GTK_ENTRY(ddata.hname), rp->hostname);
		gtk_entry_set_text(GTK_ENTRY(ddata.alias), rp->alias);
		if  (rp->ht_flags & HT_PROBEFIRST)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata.probe), TRUE);
		if  (rp->ht_flags & HT_MANUAL)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata.manual), TRUE);
		if  (rp->ht_flags & HT_TRUSTED)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata.trusted), TRUE);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(ddata.timo), (gdouble) rp->ht_timeout);
		while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK  &&  !extract_uhost_dlg(rp, &ddata))
			;
	}
	gtk_widget_destroy(dlg);
	if  (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(hlist_store), &iter, NULL, row))
		upd_hrow(rp, &iter);
}

void	cb_delete()
{
	int	row = getselectedhost();
	int	n;
	GtkTreeIter  iter;

	if  (row < 0)
		return;
	hostnum--;
	for  (n = row;  n < hostnum;  n++)
		hostlist[n] = hostlist[n+1];
	if  (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(hlist_store), &iter, NULL, row))
		gtk_list_store_remove(GTK_LIST_STORE(hlist_store), &iter);
}

void	cb_locaddr()
{
	GtkWidget  *dlg, *isloc, *locwid;

	dlg = gtk_dialog_new_with_buttons("Set local address",
					  GTK_WINDOW(toplevel),
					  GTK_DIALOG_DESTROY_WITH_PARENT,
					  GTK_STOCK_OK,
					  GTK_RESPONSE_OK,
					  GTK_STOCK_CANCEL,
					  GTK_RESPONSE_CANCEL,
					  NULL);

	isloc = gtk_check_button_new_with_label("Local address set");
	locwid = gtk_entry_new();

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), isloc, FALSE, FALSE, DEF_PAD);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), locwid, FALSE, FALSE, DEF_PAD);
	if  (hadlocaddr != NO_IPADDR)  {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(isloc), TRUE);
		gtk_entry_set_text(GTK_ENTRY(locwid), phname(myhostid, hadlocaddr));
	}
	gtk_widget_show_all(dlg);

	while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)
		if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(isloc)))  {
			const  gchar  *h = gtk_entry_get_text(GTK_ENTRY(locwid));
			netid_t  resip = 0;
			enum IPatype iptype = validhostname(h, &resip);

			if  (iptype != NO_IPADDR)  {
				hadlocaddr = iptype;
				myhostid = resip;
				lochdisplay();
				break;
			}
		}
	  gtk_widget_destroy(dlg);
}

void	cb_defuser()
{
	GtkWidget  *dlg, *dclu;

	dlg = gtk_dialog_new_with_buttons("Default client user",
					  GTK_WINDOW(toplevel),
					  GTK_DIALOG_DESTROY_WITH_PARENT,
					  GTK_STOCK_OK,
					  GTK_RESPONSE_OK,
					  GTK_STOCK_CANCEL,
					  GTK_RESPONSE_CANCEL,
					  NULL);

	dclu = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), dclu, FALSE, FALSE, DEF_PAD);
	gtk_entry_set_text(GTK_ENTRY(dclu), defcluser);
	gtk_widget_show_all(dlg);

	while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
		const gchar *u = gtk_entry_get_text(GTK_ENTRY(dclu));
		if  (strlen(u) == 0  ||  validuser(u))  {
			strncpy(defcluser, u, UIDSIZE);
			defudisplay();
			break;
		}
	}
	gtk_widget_destroy(dlg);
}

static GtkActionEntry entries[] = {
	{ "FileMenu", NULL, "_File" },
	{ "HostMenu", NULL, "_Hosts" },
	{ "OptMenu", NULL, "_Options"  },
	{ "HelpMenu", NULL, "_Help" },
	{ "Quit", GTK_STOCK_QUIT, "_Quit", "<control>Q", "Quit and save", G_CALLBACK(gtk_main_quit)},
	{ "Addh", NULL, "Add _host", "h", "Add Unix host", G_CALLBACK(cb_addhost)},
	{ "Addc", NULL, "Add _client", "c", "Add client machine name", G_CALLBACK(cb_addclient)},
	{ "Addcu", NULL, "Add client _user", "u", "Add client user name", G_CALLBACK(cb_addclientuser)},
	{ "Edit", NULL, "_Edit", "e", "Edit current line", G_CALLBACK(cb_edit)},
	{ "Del", NULL, "_Delete", "<shift>D", "Delete current line", G_CALLBACK(cb_delete)},
	{ "Locaddr", NULL, "Set _local address", "<shift>L", "Set local IP address", G_CALLBACK(cb_locaddr)},
	{ "Defu", NULL, "Default user _name", NULL, "Set default client user name", G_CALLBACK(cb_defuser)},
	{ "About", NULL, "About xhostedit", NULL, "About xbtuser", G_CALLBACK(cb_about)}  };

static char	uimenu[] =
"<ui>"
	"<menubar name='MenuBar'>"
		"<menu action='FileMenu'>"
			"<menuitem action='Quit'/>"
		"</menu>"
		"<menu action='HostMenu'>"
			"<menuitem action='Addh'/>"
			"<menuitem action='Addc'/>"
			"<menuitem action='Addcu'/>"
			"<menuitem action='Edit'/>"
			"<menuitem action='Del'/>"
		"</menu>"
		"<menu action='OptMenu'>"
			"<menuitem action='Locaddr'/>"
			"<menuitem action='Defu'/>"
		"</menu>"
		"<menu action='HelpMenu'>"
			"<menuitem action='About'/>"
		"</menu>"
	"</menubar>"
"</ui>";


void	winit()
{
	toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(toplevel), 750, 400);
	gtk_window_set_title(GTK_WINDOW(toplevel), "Editing host file");
	gtk_container_set_border_width(GTK_CONTAINER(toplevel), 5);
	gtk_window_set_resizable(GTK_WINDOW(toplevel), TRUE);
	g_signal_connect(G_OBJECT(toplevel), "delete_event", G_CALLBACK(gtk_false), NULL);
	g_signal_connect(G_OBJECT(toplevel), "destroy", G_CALLBACK(gtk_main_quit), NULL);
}

void	wstart()
{
	GError *err;
	GtkActionGroup *actions;
	GtkUIManager *ui;
	GtkTreeSelection    *sel;
	GtkWidget  *vbox, *scroll, *lab;
	GtkCellRenderer     *renderer;
	int	cnt;

	actions = gtk_action_group_new("Actions");
	gtk_action_group_add_actions(actions, entries, G_N_ELEMENTS(entries), NULL);
	ui = gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group(ui, actions, 0);
	gtk_window_add_accel_group(GTK_WINDOW(toplevel), gtk_ui_manager_get_accel_group(ui));
	if  (!gtk_ui_manager_add_ui_from_string(ui, uimenu, -1, &err))  {
		g_message("Menu build failed");
		exit(100);
	}

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(toplevel), vbox);
	gtk_box_pack_start(GTK_BOX(vbox), gtk_ui_manager_get_widget(ui, "/MenuBar"), FALSE, FALSE, 0);

	/* Create host display treeview */

	hwid = gtk_tree_view_new();

	/* Set up columns for tree view */

	renderer = gtk_cell_renderer_toggle_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(hwid), -1, "Client", renderer, "active", CLIENT_COL, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(hwid), -1, "Host/uname", renderer, "text", HOST_COL, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(hwid), -1, "Alias/wname", renderer, "text", ALIAS_COL, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(hwid), -1, "IP", renderer, "text", IP_COL, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(hwid), -1, "Deflt", renderer, "text", DEF_COL, NULL);
	renderer = gtk_cell_renderer_toggle_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(hwid), -1, "Probe", renderer, "active", PROBE_COL, NULL);
	renderer = gtk_cell_renderer_toggle_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(hwid), -1, "Manual", renderer, "active", MANUAL_COL, NULL);
	renderer = gtk_cell_renderer_toggle_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(hwid), -1, "PWchk", renderer, "active", PWCHK_COL, NULL);
	renderer = gtk_cell_renderer_toggle_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(hwid), -1, "Trust", renderer, "active", TRUST_COL, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(hwid), -1, "Timeout", renderer, "text", TIMEOUT_COL, NULL);
	for  (cnt = 0;  cnt <= TIMEOUT_COL;  cnt++)
		gtk_tree_view_column_set_resizable(gtk_tree_view_get_column(GTK_TREE_VIEW(hwid), cnt), TRUE);

	hlist_store = gtk_list_store_new(10,
					 G_TYPE_BOOLEAN,	/* Client */
					 G_TYPE_STRING,		/* Host name */
					 G_TYPE_STRING,		/* Alias */
					 G_TYPE_STRING,		/* IP */
					 G_TYPE_STRING,		/* Default mc/user */
					 G_TYPE_BOOLEAN,	/* Probe */
					 G_TYPE_BOOLEAN,	/* Manual */
					 G_TYPE_BOOLEAN,	/* Password check */
					 G_TYPE_BOOLEAN,	/* Trusted */
					 G_TYPE_UINT);		/* Timeout */

	gtk_tree_view_set_model(GTK_TREE_VIEW(hwid), GTK_TREE_MODEL(hlist_store));
	gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(hwid), TRUE);
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(hwid));
	gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);
	g_signal_connect(hwid, "row-activated", (GCallback) cb_edit, NULL);

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(scroll), 5);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll), hwid);
	gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

	lab = gtk_label_new("Local Address");
	gtk_box_pack_start(GTK_BOX(vbox), lab, FALSE, FALSE, 0);
	locwid = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(locwid), FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), locwid, FALSE, FALSE, 0);
	lab = gtk_label_new("Default user");
	gtk_box_pack_start(GTK_BOX(vbox), lab, FALSE, FALSE, 0);
	defuwid = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(defuwid), FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), defuwid, FALSE, FALSE, 0);
}

void	hdisplay()
{
	int  hcnt;

	for  (hcnt = 0;  hcnt < hostnum;  hcnt++)  {
		GtkTreeIter	iter;
		gtk_list_store_append(hlist_store, &iter);
		upd_hrow(&hostlist[hcnt], &iter);
	}
}

int	main(int argc, char *argv[])
{
	int	ch, outfd, inplace = 0;
	FILE	*outfil;
	char	*inf = (char *) 0, *outf = (char *) 0;
	extern	char	*optarg;
	extern	int	optind;

	gtk_init(&argc, &argv);
	winit();

	while  ((ch = getopt(argc, argv, "Io:s:")) != EOF)  {
		switch  (ch)  {
			default:
				doerror(toplevel, "Usage: %s [-I] [-o file] [file]", argv[0]);
				return  1;
			case  'I':
				inplace++;
				continue;
			case  'o':
				outf = optarg;
				continue;
			case  's':
				switch  (optarg[0])  {
					default:
						sort_type = SORT_NONE;
						continue;
					case  'h':
						sort_type = SORT_HNAME;
						continue;
					case  'i':
						sort_type = SORT_IP;
						continue;
				}
		}
	}

	if  (argv[optind])
		inf = argv[optind];
	else  if  (inplace)  {
		doerror(toplevel, "-I option requires file arg", (char *) 0);
		return  1;
	}

	if  (outf)  {
		if  (inplace)  {
			doerror(toplevel, "-I option and -o %s option are not compatible", outf);
			return  1;
		}
		if  (!freopen(outf, "w", stdout))  {
			doerror(toplevel, "Cannot open output file %s", outf);
			return  2;
		}
	}

	load_hostfile(inf);
	if  (hostf_errors)
		doerror(toplevel, "Warning: There were error(s) in your host file!", (char *) 0);

	if  (inplace  &&  !freopen(inf, "w", stdout))  {
		doerror(toplevel, "Cannot reopen input file %s for writing", inf);
		return  5;
	}

	sortit();

	outfd = dup(1);
	if  (!(outfil = fdopen(outfd, "w")))  {
		doerror(toplevel, "Cannot dup output file\n", (char *) 0);
		return  3;
	}

	wstart();
	hdisplay();
	lochdisplay();
	defudisplay();
	gtk_widget_show_all(toplevel);
	gtk_main();
	dump_hostfile(outfil);
	return 0;
}
