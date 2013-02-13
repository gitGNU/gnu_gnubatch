# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'convaldlg.ui'
#
# Created: Sun Nov 25 22:30:14 2012
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_convaldlg(object):
    def setupUi(self, convaldlg):
        convaldlg.setObjectName(_fromUtf8("convaldlg"))
        convaldlg.resize(400, 185)
        self.buttonBox = QtGui.QDialogButtonBox(convaldlg)
        self.buttonBox.setGeometry(QtCore.QRect(170, 130, 211, 32))
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.label = QtGui.QLabel(convaldlg)
        self.label.setGeometry(QtCore.QRect(40, 30, 241, 17))
        self.label.setObjectName(_fromUtf8("label"))
        self.conval = QtGui.QSpinBox(convaldlg)
        self.conval.setGeometry(QtCore.QRect(120, 70, 81, 27))
        self.conval.setMinimum(-1000)
        self.conval.setMaximum(1000)
        self.conval.setObjectName(_fromUtf8("conval"))

        self.retranslateUi(convaldlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), convaldlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), convaldlg.reject)
        QtCore.QMetaObject.connectSlotsByName(convaldlg)
        convaldlg.setTabOrder(self.conval, self.buttonBox)

    def retranslateUi(self, convaldlg):
        convaldlg.setWindowTitle(QtGui.QApplication.translate("convaldlg", "Constant for arith ops", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("convaldlg", "Constant for arithment operations", None, QtGui.QApplication.UnicodeUTF8))
        self.conval.setToolTip(QtGui.QApplication.translate("convaldlg", "This is the constant which will be used for\n"
"arithmetic operations on variables.", None, QtGui.QApplication.UnicodeUTF8))

