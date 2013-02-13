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

import time
import copy

from PyQt4.QtCore import *
from PyQt4.QtGui import *

import timecon
import btqwopts
import ui_timeparam
import ui_deftimeparam

class Ctimeparam(QDialog, ui_timeparam.Ui_timeparamdlg):
    def __init__(self, parent = None):
        super(Ctimeparam, self).__init__(parent)
        self.setupUi(self)
        self.initialising = False
        self.inbtr = False

    def set_enabled(self):
        """Set up which fields are enabled"""
        v = self.timeisset.isChecked()
        self.setnow.setEnabled(v)
        self.nexttime_date.setEnabled(v)
        self.nexttime_time.setEnabled(v)
        self.repstyle.setEnabled(v)
        ind = self.repstyle.currentIndex()
        v = v and (ind >= timecon.timecon.TC_MINUTES)
        if not v: self.nextrep.setText("(n/a)")
        self.avSun.setEnabled(v)
        self.avMon.setEnabled(v)
        self.avTue.setEnabled(v)
        self.avWed.setEnabled(v)
        self.avThu.setEnabled(v)
        self.avFri.setEnabled(v)
        self.avSat.setEnabled(v)
        self.avHol.setEnabled(v)
        self.ifnposs.setEnabled(v)
        self.repunits.setEnabled(v)
        v = v and (ind == timecon.timecon.TC_MONTHSB or ind == timecon.timecon.TC_MONTHSE)
        self.monthday.setEnabled(v)

    def recalc_next(self):
        tc = timecon.timecon()
        tc.copyfrom_dlg(self)
        nextt = tc.advtime()
        if nextt == 0:
            res = "(n/a)"
        else:
            res = time.ctime(nextt)
        self.nextrep.setText(res)

    def on_setnow_clicked(self, b = None):
        if b is None: return
        lt = time.localtime()
        self.nexttime_date.setSelectedDate(QDate(lt.tm_year, lt.tm_mon, lt.tm_mday))
        self.nexttime_time.setTime(QTime(lt.tm_hour, lt.tm_min, lt.tm_sec))

    def on_timeisset_stateChanged(self, b = None):
        if b is None or self.initialising: return
        if self.timeisset.isChecked():
            if self.inbtr:
                defstim = timecon.timecon()
            else:
                defstim = copy.copy(btqwopts.Options.timedefs)
                defstim.tc_nexttime = time.time()
            defstim.tc_istime = True
            defstim.init_dlg(self)
        self.set_enabled()

    def on_nexttime_date_clicked(self, b = None):
        if b is None or self.initialising: return
        self.recalc_next()

    def on_nexttime_time_timeChanged(self, b = None):
        if b is None or self.initialising: return
        self.recalc_next()

    def on_repstyle_currentIndexChanged(self, b = None):
        if b is None or not isinstance(b,int) or self.initialising: return
        if b == timecon.timecon.TC_MONTHSB:
            self.monthday.setValue(1)
        elif b == timecon.timecon.TC_MONTHSE:
            self.monthday.setValue(0)
        self.recalc_next()
        self.set_enabled()

    def on_repunits_valueChanged(self, b = None):
        if b is None or self.initialising: return
        self.recalc_next()

    def on_monthday_valueChanged(self, b = None):
        if b is None or self.initialising: return
        self.recalc_next()

    def on_avSun_stateChanged(self, b = None):
        if b is None or self.initialising: return
        self.recalc_next()

    def on_avMon_stateChanged(self, b = None):
        if b is None or self.initialising: return
        self.recalc_next()

    def on_avTue_stateChanged(self, b = None):
        if b is None or self.initialising: return
        self.recalc_next()

    def on_avWed_stateChanged(self, b = None):
        if b is None or self.initialising: return
        self.recalc_next()

    def on_avThu_stateChanged(self, b = None):
        if b is None or self.initialising: return
        self.recalc_next()

    def on_avFri_stateChanged(self, b = None):
        if b is None or self.initialising: return
        self.recalc_next()

    def on_avSat_stateChanged(self, b = None):
        if b is None or self.initialising: return
        self.recalc_next()

    def on_avHol_stateChanged(self, b = None):
        if b is None or self.initialising: return
        self.recalc_next()

class Cdeftimeparam(QDialog, ui_deftimeparam.Ui_deftimepardlg):
    def __init__(self, parent = None):
        super(Cdeftimeparam, self).__init__(parent)
        self.setupUi(self)
        self.initialising = False

    def set_enabled(self):
        """Set up which fields are enabled"""
        v = self.timeisset.isChecked()
        self.repstyle.setEnabled(v)
        ind = self.repstyle.currentIndex()
        v = v and (ind >= timecon.timecon.TC_MINUTES)
        self.avSun.setEnabled(v)
        self.avMon.setEnabled(v)
        self.avTue.setEnabled(v)
        self.avWed.setEnabled(v)
        self.avThu.setEnabled(v)
        self.avFri.setEnabled(v)
        self.avSat.setEnabled(v)
        self.avHol.setEnabled(v)
        self.ifnposs.setEnabled(v)
        self.repunits.setEnabled(v)
        v = v and (ind == timecon.timecon.TC_MONTHSB or ind == timecon.timecon.TC_MONTHSE)
        self.monthday.setEnabled(v)

    def on_timeisset_stateChanged(self, b = None):
        if b is None or self.initialising: return
        self.set_enabled()

    def on_repstyle_currentIndexChanged(self, b = None):
        if b is None or not isinstance(b,int) or self.initialising: return
        self.set_enabled()
 