#   Copyright 2013 Free Software Foundation, Inc.
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

#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

all:	ui_assvardlg.py\
	ui_jredirsdlg.py\
	ui_submhdlg.py\
	ui_progopts.py\
	ui_othersigdlg.py\
	ui_btrw_main.py\
	ui_jassdlg.py\
	ui_jobeditdlg.py\
	ui_jredirdlg.py\
	ui_portdlg.py\
	ui_varcommdlg.py\
	ui_convaldlg.py\
	ui_procpardlg.py\
	ui_jvsearchdlg.py\
	ui_servlist.py\
	ui_jobviewdlg.py\
	ui_winoptsdlg.py\
	ui_jenvsdlg.py\
	ui_fldviewdlg.py\
	ui_timelimdlg.py\
	ui_titprilldlg.py\
	ui_jssearchdlg.py\
	ui_vpermdlg.py\
	ui_mailwrtdlg.py\
	ui_jcondsdlg.py\
	ui_createvar.py\
	ui_jargsdlg.py\
	ui_hostdlg.py\
	ui_jenvdlg.py\
	ui_login.py\
	ui_btrprogopts.py\
	ui_deftimeparam.py\
	ui_jargdlg.py\
	ui_timeparam.py\
	ui_jpermdlg.py\
	ui_btqw_main.py\
	ui_upermdisp.py\
	resources_rc.py

ui_assvardlg.py:	assvardlg.ui
	pyuic4 -o $@ $?

ui_jredirsdlg.py:	jredirsdlg.ui
	pyuic4 -o $@ $?

ui_submhdlg.py:	submhdlg.ui
	pyuic4 -o $@ $?

ui_progopts.py:	progopts.ui
	pyuic4 -o $@ $?

ui_othersigdlg.py:	othersigdlg.ui
	pyuic4 -o $@ $?

ui_btrw_main.py:	btrw_main.ui
	pyuic4 -o $@ $?

ui_jassdlg.py:	jassdlg.ui
	pyuic4 -o $@ $?

ui_jobeditdlg.py:	jobeditdlg.ui
	pyuic4 -o $@ $?

ui_jredirdlg.py:	jredirdlg.ui
	pyuic4 -o $@ $?

ui_portdlg.py:	portdlg.ui
	pyuic4 -o $@ $?

ui_varcommdlg.py:	varcommdlg.ui
	pyuic4 -o $@ $?

ui_convaldlg.py:	convaldlg.ui
	pyuic4 -o $@ $?

ui_procpardlg.py:	procpardlg.ui
	pyuic4 -o $@ $?

ui_jvsearchdlg.py:	jvsearchdlg.ui
	pyuic4 -o $@ $?

ui_servlist.py:	servlist.ui
	pyuic4 -o $@ $?

ui_jobviewdlg.py:	jobviewdlg.ui
	pyuic4 -o $@ $?

ui_winoptsdlg.py:	winoptsdlg.ui
	pyuic4 -o $@ $?

ui_jenvsdlg.py:	jenvsdlg.ui
	pyuic4 -o $@ $?

ui_fldviewdlg.py:	fldviewdlg.ui
	pyuic4 -o $@ $?

ui_timelimdlg.py:	timelimdlg.ui
	pyuic4 -o $@ $?

ui_titprilldlg.py:	titprilldlg.ui
	pyuic4 -o $@ $?

ui_jssearchdlg.py:	jssearchdlg.ui
	pyuic4 -o $@ $?

ui_vpermdlg.py:	vpermdlg.ui
	pyuic4 -o $@ $?

ui_mailwrtdlg.py:	mailwrtdlg.ui
	pyuic4 -o $@ $?

ui_jcondsdlg.py:	jcondsdlg.ui
	pyuic4 -o $@ $?

ui_createvar.py:	createvar.ui
	pyuic4 -o $@ $?

ui_jargsdlg.py:	jargsdlg.ui
	pyuic4 -o $@ $?

ui_hostdlg.py:	hostdlg.ui
	pyuic4 -o $@ $?

ui_jenvdlg.py:	jenvdlg.ui
	pyuic4 -o $@ $?

ui_login.py:	login.ui
	pyuic4 -o $@ $?

ui_btrprogopts.py:	btrprogopts.ui
	pyuic4 -o $@ $?

ui_deftimeparam.py:	deftimeparam.ui
	pyuic4 -o $@ $?

ui_jargdlg.py:	jargdlg.ui
	pyuic4 -o $@ $?

ui_timeparam.py:	timeparam.ui
	pyuic4 -o $@ $?

ui_jpermdlg.py:	jpermdlg.ui
	pyuic4 -o $@ $?

ui_btqw_main.py:	btqw_main.ui
	pyuic4 -o $@ $?

ui_upermdisp.py:	upermdisp.ui
	pyuic4 -o $@ $?

resources_rc.py:	resources.qrc
	pyrcc4 -o $@ $?

clean:
	rm -f ui_*.py *_rc.py *.pyc

distclean: clean
	rm -f config.py
