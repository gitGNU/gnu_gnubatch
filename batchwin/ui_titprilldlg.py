# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'titprilldlg.ui'
#
# Created: Tue Nov 13 19:26:45 2012
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_titprilldlg(object):
    def setupUi(self, titprilldlg):
        titprilldlg.setObjectName(_fromUtf8("titprilldlg"))
        titprilldlg.resize(618, 300)
        self.buttonBox = QtGui.QDialogButtonBox(titprilldlg)
        self.buttonBox.setGeometry(QtCore.QRect(400, 250, 191, 32))
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.label = QtGui.QLabel(titprilldlg)
        self.label.setGeometry(QtCore.QRect(30, 20, 81, 17))
        self.label.setObjectName(_fromUtf8("label"))
        self.edjob = QtGui.QLabel(titprilldlg)
        self.edjob.setGeometry(QtCore.QRect(130, 20, 331, 17))
        self.edjob.setText(_fromUtf8(""))
        self.edjob.setObjectName(_fromUtf8("edjob"))
        self.label_2 = QtGui.QLabel(titprilldlg)
        self.label_2.setGeometry(QtCore.QRect(30, 70, 66, 17))
        self.label_2.setObjectName(_fromUtf8("label_2"))
        self.title = QtGui.QLineEdit(titprilldlg)
        self.title.setGeometry(QtCore.QRect(110, 70, 481, 27))
        self.title.setObjectName(_fromUtf8("title"))
        self.label_3 = QtGui.QLabel(titprilldlg)
        self.label_3.setGeometry(QtCore.QRect(30, 120, 66, 17))
        self.label_3.setObjectName(_fromUtf8("label_3"))
        self.priority = QtGui.QSpinBox(titprilldlg)
        self.priority.setGeometry(QtCore.QRect(110, 120, 61, 27))
        self.priority.setMinimum(1)
        self.priority.setMaximum(255)
        self.priority.setProperty("value", 150)
        self.priority.setObjectName(_fromUtf8("priority"))
        self.rounddll = QtGui.QPushButton(titprilldlg)
        self.rounddll.setGeometry(QtCore.QRect(200, 120, 121, 27))
        self.rounddll.setObjectName(_fromUtf8("rounddll"))
        self.roundull = QtGui.QPushButton(titprilldlg)
        self.roundull.setGeometry(QtCore.QRect(360, 120, 121, 27))
        self.roundull.setObjectName(_fromUtf8("roundull"))
        self.llev = QtGui.QSlider(titprilldlg)
        self.llev.setGeometry(QtCore.QRect(110, 170, 481, 29))
        self.llev.setMinimum(1)
        self.llev.setMaximum(65535)
        self.llev.setPageStep(100)
        self.llev.setProperty("value", 1000)
        self.llev.setOrientation(QtCore.Qt.Horizontal)
        self.llev.setTickPosition(QtGui.QSlider.TicksAbove)
        self.llev.setTickInterval(1000)
        self.llev.setObjectName(_fromUtf8("llev"))
        self.label_4 = QtGui.QLabel(titprilldlg)
        self.label_4.setGeometry(QtCore.QRect(30, 180, 66, 17))
        self.label_4.setObjectName(_fromUtf8("label_4"))
        self.label_5 = QtGui.QLabel(titprilldlg)
        self.label_5.setGeometry(QtCore.QRect(30, 230, 81, 17))
        self.label_5.setObjectName(_fromUtf8("label_5"))
        self.cmdint = QtGui.QComboBox(titprilldlg)
        self.cmdint.setGeometry(QtCore.QRect(130, 230, 141, 27))
        self.cmdint.setObjectName(_fromUtf8("cmdint"))
        self.llval = QtGui.QLabel(titprilldlg)
        self.llval.setGeometry(QtCore.QRect(510, 120, 66, 17))
        self.llval.setText(_fromUtf8(""))
        self.llval.setObjectName(_fromUtf8("llval"))

        self.retranslateUi(titprilldlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), titprilldlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), titprilldlg.reject)
        QtCore.QMetaObject.connectSlotsByName(titprilldlg)
        titprilldlg.setTabOrder(self.title, self.priority)
        titprilldlg.setTabOrder(self.priority, self.rounddll)
        titprilldlg.setTabOrder(self.rounddll, self.roundull)
        titprilldlg.setTabOrder(self.roundull, self.llev)
        titprilldlg.setTabOrder(self.llev, self.cmdint)
        titprilldlg.setTabOrder(self.cmdint, self.buttonBox)

    def retranslateUi(self, titprilldlg):
        titprilldlg.setWindowTitle(QtGui.QApplication.translate("titprilldlg", "Title, priority, load level, cmd interpreter", None, QtGui.QApplication.UnicodeUTF8))
        titprilldlg.setToolTip(QtGui.QApplication.translate("titprilldlg", "Press this button to round down the load level", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("titprilldlg", "Editing job:", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("titprilldlg", "Title:", None, QtGui.QApplication.UnicodeUTF8))
        self.title.setToolTip(QtGui.QApplication.translate("titprilldlg", "This is the job title displayed for the job", None, QtGui.QApplication.UnicodeUTF8))
        self.label_3.setText(QtGui.QApplication.translate("titprilldlg", "Priority", None, QtGui.QApplication.UnicodeUTF8))
        self.priority.setToolTip(QtGui.QApplication.translate("titprilldlg", "This is the priority for the job, which in general may be between 1 and 255.\n"
"\n"
"However you may be limited to a particular range on the server where the\n"
"job lives.", None, QtGui.QApplication.UnicodeUTF8))
        self.rounddll.setText(QtGui.QApplication.translate("titprilldlg", "Round LL down", None, QtGui.QApplication.UnicodeUTF8))
        self.roundull.setToolTip(QtGui.QApplication.translate("titprilldlg", "Press this button to round up the load level", None, QtGui.QApplication.UnicodeUTF8))
        self.roundull.setText(QtGui.QApplication.translate("titprilldlg", "Round LL up", None, QtGui.QApplication.UnicodeUTF8))
        self.llev.setToolTip(QtGui.QApplication.translate("titprilldlg", "This is the required load level.\n"
"\n"
"Note that this is reset to the default for the command interpreter\n"
"when that is changed.\n"
"\n"
"You may not be able to change this from the default if you do not\n"
"have \"special create\" permission.", None, QtGui.QApplication.UnicodeUTF8))
        self.label_4.setText(QtGui.QApplication.translate("titprilldlg", "Load Lev", None, QtGui.QApplication.UnicodeUTF8))
        self.label_5.setText(QtGui.QApplication.translate("titprilldlg", "Cmd interp", None, QtGui.QApplication.UnicodeUTF8))
        self.cmdint.setToolTip(QtGui.QApplication.translate("titprilldlg", "This is the required command interpreter", None, QtGui.QApplication.UnicodeUTF8))

