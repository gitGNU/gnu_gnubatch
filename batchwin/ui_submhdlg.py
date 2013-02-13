# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'submhdlg.ui'
#
# Created: Thu Jan 31 19:12:45 2013
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_submhdlg(object):
    def setupUi(self, submhdlg):
        submhdlg.setObjectName(_fromUtf8("submhdlg"))
        submhdlg.resize(400, 250)
        self.buttonBox = QtGui.QDialogButtonBox(submhdlg)
        self.buttonBox.setGeometry(QtCore.QRect(160, 180, 201, 32))
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.label = QtGui.QLabel(submhdlg)
        self.label.setGeometry(QtCore.QRect(60, 40, 231, 17))
        self.label.setObjectName(_fromUtf8("label"))
        self.hostname = QtGui.QComboBox(submhdlg)
        self.hostname.setGeometry(QtCore.QRect(60, 110, 231, 27))
        self.hostname.setObjectName(_fromUtf8("hostname"))

        self.retranslateUi(submhdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), submhdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), submhdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(submhdlg)
        submhdlg.setTabOrder(self.hostname, self.buttonBox)

    def retranslateUi(self, submhdlg):
        submhdlg.setWindowTitle(QtGui.QApplication.translate("submhdlg", "Host for submission of job", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("submhdlg", "Select host for submission of job", None, QtGui.QApplication.UnicodeUTF8))
        self.hostname.setToolTip(QtGui.QApplication.translate("submhdlg", "This is the recipient server for the job", None, QtGui.QApplication.UnicodeUTF8))

