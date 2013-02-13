# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'winoptsdlg.ui'
#
# Created: Tue Aug 14 11:40:54 2012
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_winoptdlg(object):
    def setupUi(self, winoptdlg):
        winoptdlg.setObjectName(_fromUtf8("winoptdlg"))
        winoptdlg.resize(602, 324)
        self.removeall = QtGui.QPushButton(winoptdlg)
        self.removeall.setGeometry(QtCore.QRect(190, 260, 181, 31))
        self.removeall.setObjectName(_fromUtf8("removeall"))
        self.buttonBox = QtGui.QDialogButtonBox(winoptdlg)
        self.buttonBox.setGeometry(QtCore.QRect(500, 60, 81, 241))
        self.buttonBox.setOrientation(QtCore.Qt.Vertical)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.label_4 = QtGui.QLabel(winoptdlg)
        self.label_4.setGeometry(QtCore.QRect(200, 20, 221, 17))
        font = QtGui.QFont()
        font.setUnderline(True)
        self.label_4.setFont(font)
        self.label_4.setObjectName(_fromUtf8("label_4"))
        self.label = QtGui.QLabel(winoptdlg)
        self.label.setGeometry(QtCore.QRect(60, 70, 91, 17))
        self.label.setObjectName(_fromUtf8("label"))
        self.queuename = QtGui.QComboBox(winoptdlg)
        self.queuename.setGeometry(QtCore.QRect(180, 60, 231, 27))
        self.queuename.setEditable(True)
        self.queuename.setObjectName(_fromUtf8("queuename"))
        self.inclnull = QtGui.QCheckBox(winoptdlg)
        self.inclnull.setGeometry(QtCore.QRect(50, 110, 411, 22))
        self.inclnull.setChecked(True)
        self.inclnull.setObjectName(_fromUtf8("inclnull"))
        self.label_2 = QtGui.QLabel(winoptdlg)
        self.label_2.setGeometry(QtCore.QRect(60, 160, 111, 17))
        self.label_2.setObjectName(_fromUtf8("label_2"))
        self.label_3 = QtGui.QLabel(winoptdlg)
        self.label_3.setGeometry(QtCore.QRect(60, 210, 101, 17))
        self.label_3.setObjectName(_fromUtf8("label_3"))
        self.username = QtGui.QComboBox(winoptdlg)
        self.username.setGeometry(QtCore.QRect(180, 150, 231, 27))
        self.username.setEditable(True)
        self.username.setObjectName(_fromUtf8("username"))
        self.groupname = QtGui.QComboBox(winoptdlg)
        self.groupname.setGeometry(QtCore.QRect(180, 210, 231, 27))
        self.groupname.setEditable(True)
        self.groupname.setObjectName(_fromUtf8("groupname"))

        self.retranslateUi(winoptdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), winoptdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), winoptdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(winoptdlg)
        winoptdlg.setTabOrder(self.queuename, self.inclnull)
        winoptdlg.setTabOrder(self.inclnull, self.username)
        winoptdlg.setTabOrder(self.username, self.groupname)
        winoptdlg.setTabOrder(self.groupname, self.removeall)
        winoptdlg.setTabOrder(self.removeall, self.buttonBox)

    def retranslateUi(self, winoptdlg):
        winoptdlg.setWindowTitle(QtGui.QApplication.translate("winoptdlg", "Window Options", None, QtGui.QApplication.UnicodeUTF8))
        self.removeall.setToolTip(QtGui.QApplication.translate("winoptdlg", "Clear all limits on view restrictions", None, QtGui.QApplication.UnicodeUTF8))
        self.removeall.setText(QtGui.QApplication.translate("winoptdlg", "Remove all restrictions", None, QtGui.QApplication.UnicodeUTF8))
        self.label_4.setText(QtGui.QApplication.translate("winoptdlg", "These are for the current window", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("winoptdlg", "Job queue", None, QtGui.QApplication.UnicodeUTF8))
        self.queuename.setToolTip(QtGui.QApplication.translate("winoptdlg", "Select a queue name to restrict the view to using the button.\n"
"You can also select lists of queue names using commas and \"shell glob\" like\n"
"matching - e.g. q*,*list", None, QtGui.QApplication.UnicodeUTF8))
        self.inclnull.setToolTip(QtGui.QApplication.translate("winoptdlg", "Select this to include jobs with no queue name at all\n"
"when you are limiting the display to a queue name\n"
"or names.", None, QtGui.QApplication.UnicodeUTF8))
        self.inclnull.setText(QtGui.QApplication.translate("winoptdlg", "Include jobs with no queue prefix when limiting by queue", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("winoptdlg", "User name(s)", None, QtGui.QApplication.UnicodeUTF8))
        self.label_3.setText(QtGui.QApplication.translate("winoptdlg", "Group Name(s)", None, QtGui.QApplication.UnicodeUTF8))
        self.username.setToolTip(QtGui.QApplication.translate("winoptdlg", "Select a user name to restrict the view to using the button.\n"
"You can also select lists of user names using commas and \"shell glob\" like\n"
"matching - e.g. j*,*smith", None, QtGui.QApplication.UnicodeUTF8))
        self.groupname.setToolTip(QtGui.QApplication.translate("winoptdlg", "Select a group name to restrict the view to using the button.\n"
"You can also select lists of group names using commas and \"shell glob\" like\n"
"matching - e.g. q*,*list", None, QtGui.QApplication.UnicodeUTF8))

