# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'timelimdlg.ui'
#
# Created: Fri Nov 16 09:31:50 2012
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_timelimdlg(object):
    def setupUi(self, timelimdlg):
        timelimdlg.setObjectName(_fromUtf8("timelimdlg"))
        timelimdlg.resize(511, 300)
        self.buttonBox = QtGui.QDialogButtonBox(timelimdlg)
        self.buttonBox.setGeometry(QtCore.QRect(260, 260, 201, 32))
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.edjob = QtGui.QLabel(timelimdlg)
        self.edjob.setGeometry(QtCore.QRect(110, 20, 331, 17))
        self.edjob.setText(_fromUtf8(""))
        self.edjob.setObjectName(_fromUtf8("edjob"))
        self.label = QtGui.QLabel(timelimdlg)
        self.label.setGeometry(QtCore.QRect(20, 20, 81, 17))
        self.label.setObjectName(_fromUtf8("label"))
        self.label_2 = QtGui.QLabel(timelimdlg)
        self.label_2.setGeometry(QtCore.QRect(20, 60, 91, 17))
        self.label_2.setObjectName(_fromUtf8("label_2"))
        self.deltime = QtGui.QSpinBox(timelimdlg)
        self.deltime.setGeometry(QtCore.QRect(140, 50, 91, 27))
        self.deltime.setMaximum(32767)
        self.deltime.setObjectName(_fromUtf8("deltime"))
        self.resetdel = QtGui.QPushButton(timelimdlg)
        self.resetdel.setGeometry(QtCore.QRect(270, 50, 98, 27))
        self.resetdel.setObjectName(_fromUtf8("resetdel"))
        self.label_3 = QtGui.QLabel(timelimdlg)
        self.label_3.setGeometry(QtCore.QRect(20, 100, 66, 17))
        self.label_3.setObjectName(_fromUtf8("label_3"))
        self.runth = QtGui.QSpinBox(timelimdlg)
        self.runth.setGeometry(QtCore.QRect(139, 90, 91, 27))
        self.runth.setMaximum(999)
        self.runth.setObjectName(_fromUtf8("runth"))
        self.runtm = QtGui.QSpinBox(timelimdlg)
        self.runtm.setGeometry(QtCore.QRect(240, 90, 71, 27))
        self.runtm.setMaximum(59)
        self.runtm.setObjectName(_fromUtf8("runtm"))
        self.runts = QtGui.QSpinBox(timelimdlg)
        self.runts.setGeometry(QtCore.QRect(320, 90, 71, 27))
        self.runts.setMaximum(59)
        self.runts.setObjectName(_fromUtf8("runts"))
        self.resetrunt = QtGui.QPushButton(timelimdlg)
        self.resetrunt.setGeometry(QtCore.QRect(410, 90, 71, 27))
        self.resetrunt.setObjectName(_fromUtf8("resetrunt"))
        self.label_4 = QtGui.QLabel(timelimdlg)
        self.label_4.setGeometry(QtCore.QRect(20, 140, 66, 17))
        self.label_4.setObjectName(_fromUtf8("label_4"))
        self.termsig = QtGui.QComboBox(timelimdlg)
        self.termsig.setGeometry(QtCore.QRect(140, 130, 171, 27))
        self.termsig.setObjectName(_fromUtf8("termsig"))
        self.label_5 = QtGui.QLabel(timelimdlg)
        self.label_5.setGeometry(QtCore.QRect(20, 180, 66, 17))
        self.label_5.setObjectName(_fromUtf8("label_5"))
        self.gtmin = QtGui.QSpinBox(timelimdlg)
        self.gtmin.setGeometry(QtCore.QRect(140, 180, 71, 27))
        self.gtmin.setMaximum(500)
        self.gtmin.setObjectName(_fromUtf8("gtmin"))
        self.gtsec = QtGui.QSpinBox(timelimdlg)
        self.gtsec.setGeometry(QtCore.QRect(240, 180, 60, 27))
        self.gtsec.setMaximum(59)
        self.gtsec.setObjectName(_fromUtf8("gtsec"))
        self.resetaddt = QtGui.QPushButton(timelimdlg)
        self.resetaddt.setGeometry(QtCore.QRect(340, 180, 71, 27))
        self.resetaddt.setObjectName(_fromUtf8("resetaddt"))

        self.retranslateUi(timelimdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), timelimdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), timelimdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(timelimdlg)
        timelimdlg.setTabOrder(self.deltime, self.resetdel)
        timelimdlg.setTabOrder(self.resetdel, self.runth)
        timelimdlg.setTabOrder(self.runth, self.runtm)
        timelimdlg.setTabOrder(self.runtm, self.runts)
        timelimdlg.setTabOrder(self.runts, self.resetrunt)
        timelimdlg.setTabOrder(self.resetrunt, self.termsig)
        timelimdlg.setTabOrder(self.termsig, self.gtmin)
        timelimdlg.setTabOrder(self.gtmin, self.gtsec)
        timelimdlg.setTabOrder(self.gtsec, self.resetaddt)
        timelimdlg.setTabOrder(self.resetaddt, self.buttonBox)

    def retranslateUi(self, timelimdlg):
        timelimdlg.setWindowTitle(QtGui.QApplication.translate("timelimdlg", "Time limits", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("timelimdlg", "Editing job:", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("timelimdlg", "Delete time", None, QtGui.QApplication.UnicodeUTF8))
        self.deltime.setToolTip(QtGui.QApplication.translate("timelimdlg", "This is the time in hours afer the first submission\n"
"after which the job will be automatically deleted.\n"
"\n"
"Set to zero to suppress automatic deletion.", None, QtGui.QApplication.UnicodeUTF8))
        self.deltime.setSuffix(QtGui.QApplication.translate("timelimdlg", " hrs", None, QtGui.QApplication.UnicodeUTF8))
        self.resetdel.setToolTip(QtGui.QApplication.translate("timelimdlg", "Reset the delete time quickly to zero", None, QtGui.QApplication.UnicodeUTF8))
        self.resetdel.setText(QtGui.QApplication.translate("timelimdlg", "Reset", None, QtGui.QApplication.UnicodeUTF8))
        self.label_3.setText(QtGui.QApplication.translate("timelimdlg", "Run time", None, QtGui.QApplication.UnicodeUTF8))
        self.runth.setToolTip(QtGui.QApplication.translate("timelimdlg", "Give maximum run time here in hours.\n"
"\n"
"Zero means no limit.", None, QtGui.QApplication.UnicodeUTF8))
        self.runth.setSuffix(QtGui.QApplication.translate("timelimdlg", " H", None, QtGui.QApplication.UnicodeUTF8))
        self.runtm.setToolTip(QtGui.QApplication.translate("timelimdlg", "Give maximum run time here in minutes.\n"
"\n"
"Zero means no limit.", None, QtGui.QApplication.UnicodeUTF8))
        self.runtm.setSuffix(QtGui.QApplication.translate("timelimdlg", " M", None, QtGui.QApplication.UnicodeUTF8))
        self.runts.setToolTip(QtGui.QApplication.translate("timelimdlg", "Give maximum run time here in seconds.\n"
"\n"
"Zero means no limit.", None, QtGui.QApplication.UnicodeUTF8))
        self.runts.setSuffix(QtGui.QApplication.translate("timelimdlg", " S", None, QtGui.QApplication.UnicodeUTF8))
        self.resetrunt.setToolTip(QtGui.QApplication.translate("timelimdlg", "Reset run time limit to zero (cancelling it)", None, QtGui.QApplication.UnicodeUTF8))
        self.resetrunt.setText(QtGui.QApplication.translate("timelimdlg", "Reset", None, QtGui.QApplication.UnicodeUTF8))
        self.label_4.setText(QtGui.QApplication.translate("timelimdlg", "Signal", None, QtGui.QApplication.UnicodeUTF8))
        self.termsig.setToolTip(QtGui.QApplication.translate("timelimdlg", "This specifies the signal to be used after the run time is exhausted.\n"
"\n"
"Unless this is Kill then the job may continue for the Grace Time until\n"
"that is exhausted.", None, QtGui.QApplication.UnicodeUTF8))
        self.label_5.setText(QtGui.QApplication.translate("timelimdlg", "Add time", None, QtGui.QApplication.UnicodeUTF8))
        self.gtmin.setToolTip(QtGui.QApplication.translate("timelimdlg", "Additional time in minutes after first kill", None, QtGui.QApplication.UnicodeUTF8))
        self.gtmin.setSuffix(QtGui.QApplication.translate("timelimdlg", " M", None, QtGui.QApplication.UnicodeUTF8))
        self.gtsec.setToolTip(QtGui.QApplication.translate("timelimdlg", "Additional time in seconds after first kill", None, QtGui.QApplication.UnicodeUTF8))
        self.gtsec.setSuffix(QtGui.QApplication.translate("timelimdlg", " S", None, QtGui.QApplication.UnicodeUTF8))
        self.resetaddt.setToolTip(QtGui.QApplication.translate("timelimdlg", "Reset additional time limit to zero", None, QtGui.QApplication.UnicodeUTF8))
        self.resetaddt.setText(QtGui.QApplication.translate("timelimdlg", "Reset", None, QtGui.QApplication.UnicodeUTF8))

