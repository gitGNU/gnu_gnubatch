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
from PyQt4.QtCore import *
import xmlutil

class timecon:
    """Time constraint values"""

    TC_DELETE = 0                   # Run once and then delete
    TC_RETAIN = 1                   # Run once and leave on Q
    TC_MINUTES = 2                  # Repeat rate - various ints
    TC_HOURS = 3
    TC_DAYS = 4
    TC_WEEKS = 5
    TC_MONTHSB = 6                  # Months relative to beginning
    TC_MONTHSE = 7                  # Months relative to end
    TC_YEARS = 8

    TC_SKIP = 0                     # Skip it, reschedule next etc
    TC_WAIT1 = 1                    # Wait but do not postpone others
    TC_WAITALL = 2                  # Wait and postpone others
    TC_CATCHUP = 3                  # Run one and catch up

    Daynames = ('Sun','Mon','Tue','Wed','Thu','Fri','Sat','Hol')
    Incrs = (60, 60*60, 60*60*24, 60*60*24*7)
    Mdays = (31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31)
    Secsperday = 60 * 60 * 24

    def __init__(self):
        self.tc_nexttime = int(time.time())         # Better set something half-sensible
        self.tc_istime = False
        self.tc_mday = 1
        self.tc_nvaldays = (1 << 6) | (1 << 0)
        self.tc_repeat = timecon.TC_RETAIN
        self.tc_nposs = timecon.TC_WAIT1
        self.tc_rate = 1

    def load(self, node):
        """Load save time details from XML file"""
        self.tc_istime = node.toElement().attribute("timeset", "n") == "y"
        child = node.firstChild()
        while not child.isNull():
            tagn = child.toElement().tagName()
            value = int(xmlutil.getText(child))
            if tagn == "nexttime":
                self.tc_nexttime = value
            elif tagn == "repeat":
                self.tc_repeat = value
            elif tagn == "rate":
                self.tc_rate = value
            elif tagn == "mday":
                self.tc_rate = value
            elif tagn == "nvaldays":
                self.tc_nvaldays = value
            elif tagn == "nposs":
                self.tc_nposs = value
            child = child.nextSibling()

    def save(self, doc, pnode, name):
        """Save time details to XML file"""
        savend = doc.createElement(name)
        pnode.appendChild(savend)
        if self.tc_istime:
            savend.setAttribute("timeset", "y")
        else:
            savend.setAttribute("timeset", "n")
        xmlutil.save_xml_string(doc, savend, "nexttime", self.tc_nexttime)
        xmlutil.save_xml_string(doc, savend, "repeat", self.tc_repeat)
        xmlutil.save_xml_string(doc, savend, "rate", self.tc_rate)
        xmlutil.save_xml_string(doc, savend, "mday", self.tc_mday)
        xmlutil.save_xml_string(doc, savend, "nvaldays", self.tc_nvaldays)
        xmlutil.save_xml_string(doc, savend, "nposs", self.tc_nposs)

    def init_dlg(self, dlg):
        """Initialise dlg assuming standardised names from the structure"""
        dlg.initialising = True
        dlg.timeisset.setChecked(self.tc_istime)
        lt = time.localtime(self.tc_nexttime)
        try:
            dlg.nexttime_date.setSelectedDate(QDate(lt.tm_year, lt.tm_mon, lt.tm_mday))
            dlg.nexttime_time.setTime(QTime(lt.tm_hour, lt.tm_min, lt.tm_sec))
        except AttributeError:
            pass
        dlg.repstyle.setCurrentIndex(self.tc_repeat)
        dlg.repunits.setValue(self.tc_rate)
        dlg.ifnposs.setCurrentIndex(self.tc_nposs)
        dlg.monthday.setValue(self.tc_mday)
        for num,day in enumerate(timecon.Daynames):
            getattr(dlg, "av" + day).setChecked((self.tc_nvaldays & (1 << num)) != 0)
        dlg.initialising = False

    def copyfrom_dlg(self, dlg):
        """Copy back from dialog"""
        self.tc_istime = dlg.timeisset.isChecked()
        try:
            # Use this for time default dialog which doesn't have a nexttime thing in
            # I believe that this takes care of timezone conversions but we'll see
            rest = QDateTime(dlg.nexttime_date.selectedDate(), dlg.nexttime_time.time())
            self.tc_nexttime = rest.toTime_t()
        except AttributeError:
            self.tc_nexttime = 0
        self.tc_repeat = dlg.repstyle.currentIndex()
        self.tc_nposs = dlg.ifnposs.currentIndex()
        self.tc_rate = dlg.repunits.value()
        self.tc_mday = dlg.monthday.value()
        self.tc_nvaldays = 0
        for num,day in enumerate(timecon.Daynames):
            if getattr(dlg, "av" + day).isChecked():
                self.tc_nvaldays |= 1 << num
        if self.tc_nvaldays == 0xff: self.tc_nvaldays = 0

    def advtime(self):
        """Calculate advance to next time

NB We need to fix this to consult a holidays table"""

        # No repeat in effect

        if not self.tc_istime or self.tc_repeat <= timecon.TC_RETAIN: return 0

        result = self.tc_nexttime

        # For repeats of simple amounts just increment

        if self.tc_repeat <= timecon.TC_WEEKS:
            result += self.tc_rate * timecon.Incrs[self.tc_repeat - timecon.TC_MINUTES]
        else:
            # Repeats of months or years

            tparts = time.localtime(result)

            if self.tc_repeat == timecon.TC_YEARS:
                # Just add to years but allow for overflow

                result = time.mktime((tparts.tm_year+self.tc_rate,tparts.tm_mon,tparts.tm_mday,tparts.tm_hour,tparts.tm_min,tparts.tm_sec,0,0,tparts.tm_isdst))
                if result >= 0x7fffffff: return 0

            elif self.tc_repeat == timecon.TC_MONTHSB:

                # Months relative to the beginning, just stuff month day in the field

                mday = self.tc_mday
                if mday <= 0: mday = 1
                result = time.mktime((tparts.tm_year, tparts.tm_mon+self.tc_rate, mday, tparts.tm_hour, tparts.tm_min, tparts.tm_sec, 0, 0, tparts.tm_isdst))

            else:       # Back from the end case
                # We put 28 as the day of the month so we can't accidentally flip over into the next month

                result = time.mktime((tparts.tm_year, tparts.tm_mon+self.tc_rate, 28, tparts.tm_hour, tparts.tm_min, tparts.tm_sec, 0, 0, tparts.tm_isdst))
                tparts = time.localtime(result)
                daysinmon = timecon.Mdays[tparts.tm_mon-1]
                if tparts.tm_mon == 2 and tparts.tm_year % 4 == 0: daysinmon += 1
                offsetdays = (daysinmon - self.tc_mday) - tparts.tm_mday        # Might be negative
                result += offsetdays * timecon.Secsperday

                # Now slide the day backwards to avoid the avoiding days
                # Only go back 14 days to avoid infinite loop

                dayofweek = (tparts.tm_wday + offsetdays + 1) % 7
                n = 14
                while  (self.tc_nvaldays & (1 << dayofweek)) != 0:
                    dayofweek -= 1
                    if dayofweek < 0: dayofweek = 6
                    result -= timecon.Secsperday
                    n -= 1
                    if n < 0: break
                return result

        # All cases other than months rel to end, slide forward

        tparts = time.localtime(result)
        n = 14
        dayofweek = (tparts.tm_wday + 1) % 7
        while (self.tc_nvaldays & (1 << dayofweek)) != 0:
            dayofweek += 1
            if dayofweek > 6: dayofweek = 0
            result += timecon.Secsperday
            n -= 1
            if n < 0: break
        if result >= 0x7fffffff: return 0
        return result
