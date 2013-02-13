# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'login.ui'
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

class Ui_logindlg(object):
    def setupUi(self, logindlg):
        logindlg.setObjectName(_fromUtf8("logindlg"))
        logindlg.resize(400, 300)
        self.buttonBox = QtGui.QDialogButtonBox(logindlg)
        self.buttonBox.setGeometry(QtCore.QRect(30, 240, 341, 32))
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.layoutWidget = QtGui.QWidget(logindlg)
        self.layoutWidget.setGeometry(QtCore.QRect(60, 20, 258, 191))
        self.layoutWidget.setObjectName(_fromUtf8("layoutWidget"))
        self.gridLayout = QtGui.QGridLayout(self.layoutWidget)
        self.gridLayout.setMargin(0)
        self.gridLayout.setObjectName(_fromUtf8("gridLayout"))
        self.label = QtGui.QLabel(self.layoutWidget)
        self.label.setObjectName(_fromUtf8("label"))
        self.gridLayout.addWidget(self.label, 0, 0, 1, 1)
        self.servName = QtGui.QLineEdit(self.layoutWidget)
        self.servName.setReadOnly(True)
        self.servName.setObjectName(_fromUtf8("servName"))
        self.gridLayout.addWidget(self.servName, 0, 1, 1, 1)
        self.label_2 = QtGui.QLabel(self.layoutWidget)
        self.label_2.setObjectName(_fromUtf8("label_2"))
        self.gridLayout.addWidget(self.label_2, 1, 0, 1, 1)
        self.clientName = QtGui.QLineEdit(self.layoutWidget)
        self.clientName.setReadOnly(True)
        self.clientName.setObjectName(_fromUtf8("clientName"))
        self.gridLayout.addWidget(self.clientName, 1, 1, 1, 1)
        self.label_3 = QtGui.QLabel(self.layoutWidget)
        self.label_3.setObjectName(_fromUtf8("label_3"))
        self.gridLayout.addWidget(self.label_3, 2, 0, 1, 1)
        self.userName = QtGui.QLineEdit(self.layoutWidget)
        self.userName.setObjectName(_fromUtf8("userName"))
        self.gridLayout.addWidget(self.userName, 2, 1, 1, 1)
        self.label_4 = QtGui.QLabel(self.layoutWidget)
        self.label_4.setObjectName(_fromUtf8("label_4"))
        self.gridLayout.addWidget(self.label_4, 3, 0, 1, 1)
        self.password = QtGui.QLineEdit(self.layoutWidget)
        self.password.setEchoMode(QtGui.QLineEdit.Password)
        self.password.setObjectName(_fromUtf8("password"))
        self.gridLayout.addWidget(self.password, 3, 1, 1, 1)
        self.label.setBuddy(self.servName)
        self.label_2.setBuddy(self.clientName)
        self.label_3.setBuddy(self.userName)
        self.label_4.setBuddy(self.password)

        self.retranslateUi(logindlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), logindlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), logindlg.reject)
        QtCore.QMetaObject.connectSlotsByName(logindlg)
        logindlg.setTabOrder(self.servName, self.clientName)
        logindlg.setTabOrder(self.clientName, self.userName)
        logindlg.setTabOrder(self.userName, self.password)
        logindlg.setTabOrder(self.password, self.buttonBox)

    def retranslateUi(self, logindlg):
        logindlg.setWindowTitle(QtGui.QApplication.translate("logindlg", "Login as user", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("logindlg", "Log in to server", None, QtGui.QApplication.UnicodeUTF8))
        self.servName.setToolTip(QtGui.QApplication.translate("logindlg", "This is the server being logged in to (don\'t set it here)", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("logindlg", "This client name", None, QtGui.QApplication.UnicodeUTF8))
        self.clientName.setToolTip(QtGui.QApplication.translate("logindlg", "This is the name of this machine", None, QtGui.QApplication.UnicodeUTF8))
        self.label_3.setText(QtGui.QApplication.translate("logindlg", "&User name", None, QtGui.QApplication.UnicodeUTF8))
        self.userName.setToolTip(QtGui.QApplication.translate("logindlg", "User name to log in as", None, QtGui.QApplication.UnicodeUTF8))
        self.label_4.setText(QtGui.QApplication.translate("logindlg", "&Password", None, QtGui.QApplication.UnicodeUTF8))
        self.password.setToolTip(QtGui.QApplication.translate("logindlg", "Password (if required)", None, QtGui.QApplication.UnicodeUTF8))

