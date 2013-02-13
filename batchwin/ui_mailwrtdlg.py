# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'mailwrtdlg.ui'
#
# Created: Fri Nov 16 09:49:02 2012
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_mailwrtdlg(object):
    def setupUi(self, mailwrtdlg):
        mailwrtdlg.setObjectName(_fromUtf8("mailwrtdlg"))
        mailwrtdlg.resize(400, 300)
        self.buttonBox = QtGui.QDialogButtonBox(mailwrtdlg)
        self.buttonBox.setGeometry(QtCore.QRect(170, 240, 201, 32))
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.mailresult = QtGui.QCheckBox(mailwrtdlg)
        self.mailresult.setGeometry(QtCore.QRect(40, 110, 221, 22))
        self.mailresult.setObjectName(_fromUtf8("mailresult"))
        self.writeresult = QtGui.QCheckBox(mailwrtdlg)
        self.writeresult.setGeometry(QtCore.QRect(40, 180, 261, 22))
        self.writeresult.setObjectName(_fromUtf8("writeresult"))
        self.edjob = QtGui.QLabel(mailwrtdlg)
        self.edjob.setGeometry(QtCore.QRect(130, 40, 191, 20))
        self.edjob.setText(_fromUtf8(""))
        self.edjob.setObjectName(_fromUtf8("edjob"))
        self.label = QtGui.QLabel(mailwrtdlg)
        self.label.setGeometry(QtCore.QRect(20, 40, 81, 17))
        self.label.setObjectName(_fromUtf8("label"))

        self.retranslateUi(mailwrtdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), mailwrtdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), mailwrtdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(mailwrtdlg)
        mailwrtdlg.setTabOrder(self.mailresult, self.writeresult)
        mailwrtdlg.setTabOrder(self.writeresult, self.buttonBox)

    def retranslateUi(self, mailwrtdlg):
        mailwrtdlg.setWindowTitle(QtGui.QApplication.translate("mailwrtdlg", "Mail or display completion messages", None, QtGui.QApplication.UnicodeUTF8))
        self.mailresult.setToolTip(QtGui.QApplication.translate("mailwrtdlg", "Send mail messages to user about job completion", None, QtGui.QApplication.UnicodeUTF8))
        self.mailresult.setText(QtGui.QApplication.translate("mailwrtdlg", "Mail completion message", None, QtGui.QApplication.UnicodeUTF8))
        self.writeresult.setToolTip(QtGui.QApplication.translate("mailwrtdlg", "Send messages to user screen about job completion", None, QtGui.QApplication.UnicodeUTF8))
        self.writeresult.setText(QtGui.QApplication.translate("mailwrtdlg", "Display completion message", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("mailwrtdlg", "Editing job:", None, QtGui.QApplication.UnicodeUTF8))

