# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'varcommdlg.ui'
#
# Created: Sun Nov 25 22:42:04 2012
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_varcommdlg(object):
    def setupUi(self, varcommdlg):
        varcommdlg.setObjectName(_fromUtf8("varcommdlg"))
        varcommdlg.resize(580, 225)
        self.buttonBox = QtGui.QDialogButtonBox(varcommdlg)
        self.buttonBox.setGeometry(QtCore.QRect(320, 160, 201, 32))
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.label = QtGui.QLabel(varcommdlg)
        self.label.setGeometry(QtCore.QRect(40, 40, 141, 17))
        self.label.setObjectName(_fromUtf8("label"))
        self.edvar = QtGui.QLabel(varcommdlg)
        self.edvar.setGeometry(QtCore.QRect(180, 40, 301, 17))
        self.edvar.setText(_fromUtf8(""))
        self.edvar.setObjectName(_fromUtf8("edvar"))
        self.comment = QtGui.QLineEdit(varcommdlg)
        self.comment.setGeometry(QtCore.QRect(100, 100, 381, 27))
        self.comment.setMaxLength(41)
        self.comment.setObjectName(_fromUtf8("comment"))

        self.retranslateUi(varcommdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), varcommdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), varcommdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(varcommdlg)
        varcommdlg.setTabOrder(self.comment, self.buttonBox)

    def retranslateUi(self, varcommdlg):
        varcommdlg.setWindowTitle(QtGui.QApplication.translate("varcommdlg", "Description/Comment on variable", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("varcommdlg", "Description of var", None, QtGui.QApplication.UnicodeUTF8))
        self.comment.setToolTip(QtGui.QApplication.translate("varcommdlg", "This is a description or comment about the variable", None, QtGui.QApplication.UnicodeUTF8))

