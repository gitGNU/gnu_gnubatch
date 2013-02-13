# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'jargdlg.ui'
#
# Created: Mon Nov 19 13:03:21 2012
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_jargdlg(object):
    def setupUi(self, jargdlg):
        jargdlg.setObjectName(_fromUtf8("jargdlg"))
        jargdlg.resize(560, 223)
        self.buttonBox = QtGui.QDialogButtonBox(jargdlg)
        self.buttonBox.setGeometry(QtCore.QRect(210, 160, 341, 32))
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.edjob = QtGui.QLabel(jargdlg)
        self.edjob.setGeometry(QtCore.QRect(140, 40, 331, 17))
        self.edjob.setText(_fromUtf8(""))
        self.edjob.setObjectName(_fromUtf8("edjob"))
        self.label = QtGui.QLabel(jargdlg)
        self.label.setGeometry(QtCore.QRect(40, 40, 91, 17))
        self.label.setObjectName(_fromUtf8("label"))
        self.argument = QtGui.QLineEdit(jargdlg)
        self.argument.setGeometry(QtCore.QRect(40, 80, 491, 27))
        font = QtGui.QFont()
        font.setFamily(_fromUtf8("Courier New"))
        self.argument.setFont(font)
        self.argument.setObjectName(_fromUtf8("argument"))

        self.retranslateUi(jargdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), jargdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), jargdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(jargdlg)
        jargdlg.setTabOrder(self.argument, self.buttonBox)

    def retranslateUi(self, jargdlg):
        jargdlg.setWindowTitle(QtGui.QApplication.translate("jargdlg", "Job argument", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("jargdlg", "Arg for job:", None, QtGui.QApplication.UnicodeUTF8))
        self.argument.setToolTip(QtGui.QApplication.translate("jargdlg", "This is the argument string, remember that environment variables $name\n"
"will be subsituted, together with %t for the job title and other data.", None, QtGui.QApplication.UnicodeUTF8))

