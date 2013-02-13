# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'hostdlg.ui'
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

class Ui_hostdlg(object):
    def setupUi(self, hostdlg):
        hostdlg.setObjectName(_fromUtf8("hostdlg"))
        hostdlg.resize(400, 237)
        self.buttonBox = QtGui.QDialogButtonBox(hostdlg)
        self.buttonBox.setGeometry(QtCore.QRect(40, 190, 341, 32))
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.layoutWidget = QtGui.QWidget(hostdlg)
        self.layoutWidget.setGeometry(QtCore.QRect(20, 10, 361, 121))
        self.layoutWidget.setObjectName(_fromUtf8("layoutWidget"))
        self.gridLayout = QtGui.QGridLayout(self.layoutWidget)
        self.gridLayout.setMargin(0)
        self.gridLayout.setObjectName(_fromUtf8("gridLayout"))
        self.label = QtGui.QLabel(self.layoutWidget)
        self.label.setObjectName(_fromUtf8("label"))
        self.gridLayout.addWidget(self.label, 0, 0, 1, 1)
        self.hostName = QtGui.QLineEdit(self.layoutWidget)
        self.hostName.setObjectName(_fromUtf8("hostName"))
        self.gridLayout.addWidget(self.hostName, 0, 1, 1, 2)
        self.label_2 = QtGui.QLabel(self.layoutWidget)
        self.label_2.setObjectName(_fromUtf8("label_2"))
        self.gridLayout.addWidget(self.label_2, 1, 0, 1, 1)
        self.aliasName = QtGui.QLineEdit(self.layoutWidget)
        self.aliasName.setObjectName(_fromUtf8("aliasName"))
        self.gridLayout.addWidget(self.aliasName, 1, 1, 1, 2)
        self.autoconn = QtGui.QCheckBox(hostdlg)
        self.autoconn.setGeometry(QtCore.QRect(20, 150, 191, 22))
        self.autoconn.setObjectName(_fromUtf8("autoconn"))
        self.label.setBuddy(self.hostName)
        self.label_2.setBuddy(self.aliasName)

        self.retranslateUi(hostdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), hostdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), hostdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(hostdlg)
        hostdlg.setTabOrder(self.hostName, self.aliasName)
        hostdlg.setTabOrder(self.aliasName, self.autoconn)
        hostdlg.setTabOrder(self.autoconn, self.buttonBox)

    def retranslateUi(self, hostdlg):
        hostdlg.setWindowTitle(QtGui.QApplication.translate("hostdlg", "Host name", None, QtGui.QApplication.UnicodeUTF8))
        self.buttonBox.setToolTip(QtGui.QApplication.translate("hostdlg", "Attempt to connect to this server at startup", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("hostdlg", "&Host", None, QtGui.QApplication.UnicodeUTF8))
        self.hostName.setToolTip(QtGui.QApplication.translate("hostdlg", "This is the host name to connect to or an IP address", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("hostdlg", "&Alias", None, QtGui.QApplication.UnicodeUTF8))
        self.aliasName.setToolTip(QtGui.QApplication.translate("hostdlg", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:\'Bitstream Vera Sans\'; font-size:10pt; font-weight:400; font-style:normal;\">\n"
"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Shorter alias for host name - <span style=\" font-weight:600;\">must</span> have it if host is IP</p></body></html>", None, QtGui.QApplication.UnicodeUTF8))
        self.autoconn.setText(QtGui.QApplication.translate("hostdlg", "Try to connect at startup", None, QtGui.QApplication.UnicodeUTF8))

