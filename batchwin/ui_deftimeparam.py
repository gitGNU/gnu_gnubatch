# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'deftimeparam.ui'
#
# Created: Tue Nov 13 07:42:02 2012
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_deftimepardlg(object):
    def setupUi(self, deftimepardlg):
        deftimepardlg.setObjectName(_fromUtf8("deftimepardlg"))
        deftimepardlg.resize(754, 321)
        self.buttonBox = QtGui.QDialogButtonBox(deftimepardlg)
        self.buttonBox.setGeometry(QtCore.QRect(460, 260, 241, 32))
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.repunits = QtGui.QSpinBox(deftimepardlg)
        self.repunits.setGeometry(QtCore.QRect(90, 70, 60, 27))
        self.repunits.setMinimum(1)
        self.repunits.setMaximum(60)
        self.repunits.setProperty("value", 1)
        self.repunits.setObjectName(_fromUtf8("repunits"))
        self.label_3 = QtGui.QLabel(deftimepardlg)
        self.label_3.setGeometry(QtCore.QRect(90, 230, 111, 17))
        self.label_3.setObjectName(_fromUtf8("label_3"))
        self.ifnposs = QtGui.QComboBox(deftimepardlg)
        self.ifnposs.setGeometry(QtCore.QRect(240, 220, 151, 27))
        self.ifnposs.setObjectName(_fromUtf8("ifnposs"))
        self.ifnposs.addItem(_fromUtf8(""))
        self.ifnposs.addItem(_fromUtf8(""))
        self.ifnposs.addItem(_fromUtf8(""))
        self.ifnposs.addItem(_fromUtf8(""))
        self.label_4 = QtGui.QLabel(deftimepardlg)
        self.label_4.setGeometry(QtCore.QRect(520, 40, 141, 17))
        self.label_4.setObjectName(_fromUtf8("label_4"))
        self.monthday = QtGui.QSpinBox(deftimepardlg)
        self.monthday.setGeometry(QtCore.QRect(520, 70, 60, 27))
        self.monthday.setMinimum(0)
        self.monthday.setMaximum(31)
        self.monthday.setSingleStep(1)
        self.monthday.setProperty("value", 1)
        self.monthday.setObjectName(_fromUtf8("monthday"))
        self.groupBox = QtGui.QGroupBox(deftimepardlg)
        self.groupBox.setGeometry(QtCore.QRect(90, 110, 411, 91))
        self.groupBox.setObjectName(_fromUtf8("groupBox"))
        self.avSun = QtGui.QCheckBox(self.groupBox)
        self.avSun.setGeometry(QtCore.QRect(0, 30, 97, 22))
        self.avSun.setObjectName(_fromUtf8("avSun"))
        self.avMon = QtGui.QCheckBox(self.groupBox)
        self.avMon.setGeometry(QtCore.QRect(90, 30, 97, 22))
        self.avMon.setObjectName(_fromUtf8("avMon"))
        self.avTue = QtGui.QCheckBox(self.groupBox)
        self.avTue.setGeometry(QtCore.QRect(180, 30, 97, 22))
        self.avTue.setObjectName(_fromUtf8("avTue"))
        self.avWed = QtGui.QCheckBox(self.groupBox)
        self.avWed.setGeometry(QtCore.QRect(270, 30, 111, 22))
        self.avWed.setObjectName(_fromUtf8("avWed"))
        self.avThu = QtGui.QCheckBox(self.groupBox)
        self.avThu.setGeometry(QtCore.QRect(0, 60, 97, 22))
        self.avThu.setObjectName(_fromUtf8("avThu"))
        self.avFri = QtGui.QCheckBox(self.groupBox)
        self.avFri.setGeometry(QtCore.QRect(90, 60, 97, 22))
        self.avFri.setObjectName(_fromUtf8("avFri"))
        self.avSat = QtGui.QCheckBox(self.groupBox)
        self.avSat.setGeometry(QtCore.QRect(180, 60, 97, 22))
        self.avSat.setObjectName(_fromUtf8("avSat"))
        self.avHol = QtGui.QCheckBox(self.groupBox)
        self.avHol.setGeometry(QtCore.QRect(270, 60, 97, 22))
        self.avHol.setObjectName(_fromUtf8("avHol"))
        self.repstyle = QtGui.QComboBox(deftimepardlg)
        self.repstyle.setGeometry(QtCore.QRect(220, 70, 261, 27))
        self.repstyle.setObjectName(_fromUtf8("repstyle"))
        self.repstyle.addItem(_fromUtf8(""))
        self.repstyle.addItem(_fromUtf8(""))
        self.repstyle.addItem(_fromUtf8(""))
        self.repstyle.addItem(_fromUtf8(""))
        self.repstyle.addItem(_fromUtf8(""))
        self.repstyle.addItem(_fromUtf8(""))
        self.repstyle.addItem(_fromUtf8(""))
        self.repstyle.addItem(_fromUtf8(""))
        self.repstyle.addItem(_fromUtf8(""))
        self.timeisset = QtGui.QCheckBox(deftimepardlg)
        self.timeisset.setGeometry(QtCore.QRect(70, 30, 201, 22))
        self.timeisset.setObjectName(_fromUtf8("timeisset"))

        self.retranslateUi(deftimepardlg)
        self.ifnposs.setCurrentIndex(1)
        self.repstyle.setCurrentIndex(1)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), deftimepardlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), deftimepardlg.reject)
        QtCore.QMetaObject.connectSlotsByName(deftimepardlg)
        deftimepardlg.setTabOrder(self.timeisset, self.repunits)
        deftimepardlg.setTabOrder(self.repunits, self.repstyle)
        deftimepardlg.setTabOrder(self.repstyle, self.monthday)
        deftimepardlg.setTabOrder(self.monthday, self.avSun)
        deftimepardlg.setTabOrder(self.avSun, self.avMon)
        deftimepardlg.setTabOrder(self.avMon, self.avTue)
        deftimepardlg.setTabOrder(self.avTue, self.avWed)
        deftimepardlg.setTabOrder(self.avWed, self.avThu)
        deftimepardlg.setTabOrder(self.avThu, self.avFri)
        deftimepardlg.setTabOrder(self.avFri, self.avSat)
        deftimepardlg.setTabOrder(self.avSat, self.avHol)
        deftimepardlg.setTabOrder(self.avHol, self.ifnposs)
        deftimepardlg.setTabOrder(self.ifnposs, self.buttonBox)

    def retranslateUi(self, deftimepardlg):
        deftimepardlg.setWindowTitle(QtGui.QApplication.translate("deftimepardlg", "Default time parameters", None, QtGui.QApplication.UnicodeUTF8))
        self.repunits.setToolTip(QtGui.QApplication.translate("deftimepardlg", "Number of units of the repeat given", None, QtGui.QApplication.UnicodeUTF8))
        self.label_3.setText(QtGui.QApplication.translate("deftimepardlg", "If not possible", None, QtGui.QApplication.UnicodeUTF8))
        self.ifnposs.setItemText(0, QtGui.QApplication.translate("deftimepardlg", "Skip to next", None, QtGui.QApplication.UnicodeUTF8))
        self.ifnposs.setItemText(1, QtGui.QApplication.translate("deftimepardlg", "Delay current", None, QtGui.QApplication.UnicodeUTF8))
        self.ifnposs.setItemText(2, QtGui.QApplication.translate("deftimepardlg", "Delay and adjust", None, QtGui.QApplication.UnicodeUTF8))
        self.ifnposs.setItemText(3, QtGui.QApplication.translate("deftimepardlg", "Catch up", None, QtGui.QApplication.UnicodeUTF8))
        self.label_4.setText(QtGui.QApplication.translate("deftimepardlg", "D of M or from end", None, QtGui.QApplication.UnicodeUTF8))
        self.monthday.setToolTip(QtGui.QApplication.translate("deftimepardlg", "Set the target day of the month for monthly repeats\n"
"\n"
"For month relative to the beginning repeats, this gives the target day of the\n"
"month. If this is not acceptable, then the following day is tried until acceptable.\n"
"\n"
"For month relative to the end repeats this gives the number of days back from\n"
"the end of the month, possibly zero days, so if the month has 31 days, 0 would\n"
"aim for 31st, 1 would aim for 30th and so on. If the day of the week thereby\n"
"selected is unacceptable the preceding day is tried until acceptable.\n"
"\n"
"Note that the display of month days in month relative to the end has been\n"
"changed relative to previous software hopefully to make it more\n"
"understandable.", None, QtGui.QApplication.UnicodeUTF8))
        self.groupBox.setToolTip(QtGui.QApplication.translate("deftimepardlg", "Select days to avoid when calculating repeat", None, QtGui.QApplication.UnicodeUTF8))
        self.groupBox.setTitle(QtGui.QApplication.translate("deftimepardlg", "Avoiding days", None, QtGui.QApplication.UnicodeUTF8))
        self.avSun.setToolTip(QtGui.QApplication.translate("deftimepardlg", "Set Sunday as a day to avoid when calculating repeats", None, QtGui.QApplication.UnicodeUTF8))
        self.avSun.setText(QtGui.QApplication.translate("deftimepardlg", "Sunday", None, QtGui.QApplication.UnicodeUTF8))
        self.avMon.setToolTip(QtGui.QApplication.translate("deftimepardlg", "Set Monday as a day to avoid when calculating repeats", None, QtGui.QApplication.UnicodeUTF8))
        self.avMon.setText(QtGui.QApplication.translate("deftimepardlg", "Monday", None, QtGui.QApplication.UnicodeUTF8))
        self.avTue.setToolTip(QtGui.QApplication.translate("deftimepardlg", "Set Tuesday as a day to avoid when calculating repeats", None, QtGui.QApplication.UnicodeUTF8))
        self.avTue.setText(QtGui.QApplication.translate("deftimepardlg", "Tuesday", None, QtGui.QApplication.UnicodeUTF8))
        self.avWed.setToolTip(QtGui.QApplication.translate("deftimepardlg", "Set Wednesday as a day to avoid when calculating repeats", None, QtGui.QApplication.UnicodeUTF8))
        self.avWed.setText(QtGui.QApplication.translate("deftimepardlg", "Wednesday", None, QtGui.QApplication.UnicodeUTF8))
        self.avThu.setToolTip(QtGui.QApplication.translate("deftimepardlg", "Set Thursday as a day to avoid when calculating repeats", None, QtGui.QApplication.UnicodeUTF8))
        self.avThu.setText(QtGui.QApplication.translate("deftimepardlg", "Thursday", None, QtGui.QApplication.UnicodeUTF8))
        self.avFri.setToolTip(QtGui.QApplication.translate("deftimepardlg", "Set Friday as a day to avoid when calculating repeats", None, QtGui.QApplication.UnicodeUTF8))
        self.avFri.setText(QtGui.QApplication.translate("deftimepardlg", "Friday", None, QtGui.QApplication.UnicodeUTF8))
        self.avSat.setToolTip(QtGui.QApplication.translate("deftimepardlg", "Set Saturday as a day to avoid when calculating repeats", None, QtGui.QApplication.UnicodeUTF8))
        self.avSat.setText(QtGui.QApplication.translate("deftimepardlg", "Saturday", None, QtGui.QApplication.UnicodeUTF8))
        self.avHol.setToolTip(QtGui.QApplication.translate("deftimepardlg", "Avoid one of the table of holidays when calculating repeats.\n"
"", None, QtGui.QApplication.UnicodeUTF8))
        self.avHol.setText(QtGui.QApplication.translate("deftimepardlg", "Holiday", None, QtGui.QApplication.UnicodeUTF8))
        self.repstyle.setToolTip(QtGui.QApplication.translate("deftimepardlg", "Set here what style of repeat you require.\n"
"Either set to delete at the end, to retain as \"done\" with manual reset or as\n"
"auto-repeat by the intervals given and number of units as given in the box.", None, QtGui.QApplication.UnicodeUTF8))
        self.repstyle.setItemText(0, QtGui.QApplication.translate("deftimepardlg", "Delete at end", None, QtGui.QApplication.UnicodeUTF8))
        self.repstyle.setItemText(1, QtGui.QApplication.translate("deftimepardlg", "Retain as done", None, QtGui.QApplication.UnicodeUTF8))
        self.repstyle.setItemText(2, QtGui.QApplication.translate("deftimepardlg", "Minutes", None, QtGui.QApplication.UnicodeUTF8))
        self.repstyle.setItemText(3, QtGui.QApplication.translate("deftimepardlg", "Hours", None, QtGui.QApplication.UnicodeUTF8))
        self.repstyle.setItemText(4, QtGui.QApplication.translate("deftimepardlg", "Days", None, QtGui.QApplication.UnicodeUTF8))
        self.repstyle.setItemText(5, QtGui.QApplication.translate("deftimepardlg", "Weeks", None, QtGui.QApplication.UnicodeUTF8))
        self.repstyle.setItemText(6, QtGui.QApplication.translate("deftimepardlg", "Months (relative to the beginning)", None, QtGui.QApplication.UnicodeUTF8))
        self.repstyle.setItemText(7, QtGui.QApplication.translate("deftimepardlg", "Months (relative to the end)", None, QtGui.QApplication.UnicodeUTF8))
        self.repstyle.setItemText(8, QtGui.QApplication.translate("deftimepardlg", "Years", None, QtGui.QApplication.UnicodeUTF8))
        self.timeisset.setToolTip(QtGui.QApplication.translate("deftimepardlg", "Indicate job has time set as opposed to being a \"do when possible\" job.", None, QtGui.QApplication.UnicodeUTF8))
        self.timeisset.setText(QtGui.QApplication.translate("deftimepardlg", "Set up a time parameter", None, QtGui.QApplication.UnicodeUTF8))

