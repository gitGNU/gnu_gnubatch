# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'fldviewdlg.ui'
#
# Created: Fri Oct 26 14:04:46 2012
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_fldviewdlg(object):
    def setupUi(self, fldviewdlg):
        fldviewdlg.setObjectName(_fromUtf8("fldviewdlg"))
        fldviewdlg.resize(460, 481)
        self.buttonBox = QtGui.QDialogButtonBox(fldviewdlg)
        self.buttonBox.setGeometry(QtCore.QRect(360, 10, 81, 71))
        self.buttonBox.setOrientation(QtCore.Qt.Vertical)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.label = QtGui.QLabel(fldviewdlg)
        self.label.setGeometry(QtCore.QRect(30, 30, 91, 17))
        self.label.setObjectName(_fromUtf8("label"))
        self.jorv = QtGui.QLabel(fldviewdlg)
        self.jorv.setGeometry(QtCore.QRect(130, 30, 101, 17))
        self.jorv.setObjectName(_fromUtf8("jorv"))
        self.possattr = QtGui.QComboBox(fldviewdlg)
        self.possattr.setGeometry(QtCore.QRect(30, 80, 381, 27))
        self.possattr.setObjectName(_fromUtf8("possattr"))
        self.currattr = QtGui.QListWidget(fldviewdlg)
        self.currattr.setGeometry(QtCore.QRect(30, 130, 381, 271))
        self.currattr.setDragDropMode(QtGui.QAbstractItemView.InternalMove)
        self.currattr.setObjectName(_fromUtf8("currattr"))
        self.addattr = QtGui.QPushButton(fldviewdlg)
        self.addattr.setGeometry(QtCore.QRect(30, 430, 93, 27))
        self.addattr.setObjectName(_fromUtf8("addattr"))
        self.delattr = QtGui.QPushButton(fldviewdlg)
        self.delattr.setGeometry(QtCore.QRect(310, 430, 93, 27))
        self.delattr.setObjectName(_fromUtf8("delattr"))

        self.retranslateUi(fldviewdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), fldviewdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), fldviewdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(fldviewdlg)
        fldviewdlg.setTabOrder(self.possattr, self.currattr)
        fldviewdlg.setTabOrder(self.currattr, self.addattr)
        fldviewdlg.setTabOrder(self.addattr, self.delattr)
        fldviewdlg.setTabOrder(self.delattr, self.buttonBox)

    def retranslateUi(self, fldviewdlg):
        fldviewdlg.setWindowTitle(QtGui.QApplication.translate("fldviewdlg", "Select view attributes", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("fldviewdlg", "Attributes of", None, QtGui.QApplication.UnicodeUTF8))
        self.jorv.setText(QtGui.QApplication.translate("fldviewdlg", "jorp", None, QtGui.QApplication.UnicodeUTF8))
        self.possattr.setToolTip(QtGui.QApplication.translate("fldviewdlg", "Possible attribtutes for display", None, QtGui.QApplication.UnicodeUTF8))
        self.currattr.setToolTip(QtGui.QApplication.translate("fldviewdlg", "List of attributes of job or variable to be displayed in column order", None, QtGui.QApplication.UnicodeUTF8))
        self.addattr.setToolTip(QtGui.QApplication.translate("fldviewdlg", "Add attribute to list", None, QtGui.QApplication.UnicodeUTF8))
        self.addattr.setText(QtGui.QApplication.translate("fldviewdlg", "Add", None, QtGui.QApplication.UnicodeUTF8))
        self.delattr.setToolTip(QtGui.QApplication.translate("fldviewdlg", "Delete selected attribute from list", None, QtGui.QApplication.UnicodeUTF8))
        self.delattr.setText(QtGui.QApplication.translate("fldviewdlg", "Delete", None, QtGui.QApplication.UnicodeUTF8))

