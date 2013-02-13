# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'assvardlg.ui'
#
# Created: Sun Nov 25 22:03:57 2012
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_assvardlg(object):
    def setupUi(self, assvardlg):
        assvardlg.setObjectName(_fromUtf8("assvardlg"))
        assvardlg.resize(507, 234)
        self.buttonBox = QtGui.QDialogButtonBox(assvardlg)
        self.buttonBox.setGeometry(QtCore.QRect(300, 170, 191, 32))
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.label = QtGui.QLabel(assvardlg)
        self.label.setGeometry(QtCore.QRect(10, 20, 101, 17))
        self.label.setObjectName(_fromUtf8("label"))
        self.edvar = QtGui.QLabel(assvardlg)
        self.edvar.setGeometry(QtCore.QRect(110, 20, 301, 17))
        self.edvar.setText(_fromUtf8(""))
        self.edvar.setObjectName(_fromUtf8("edvar"))
        self.textval = QtGui.QLineEdit(assvardlg)
        self.textval.setEnabled(False)
        self.textval.setGeometry(QtCore.QRect(130, 110, 347, 28))
        font = QtGui.QFont()
        font.setFamily(_fromUtf8("Courier New"))
        font.setBold(False)
        font.setItalic(False)
        font.setWeight(50)
        self.textval.setFont(font)
        self.textval.setMaxLength(49)
        self.textval.setObjectName(_fromUtf8("textval"))
        self.label_3 = QtGui.QLabel(assvardlg)
        self.label_3.setGeometry(QtCore.QRect(30, 70, 51, 17))
        self.label_3.setObjectName(_fromUtf8("label_3"))
        self.istext = QtGui.QCheckBox(assvardlg)
        self.istext.setGeometry(QtCore.QRect(290, 70, 131, 22))
        self.istext.setObjectName(_fromUtf8("istext"))
        self.intval = QtGui.QSpinBox(assvardlg)
        self.intval.setGeometry(QtCore.QRect(130, 70, 101, 27))
        self.intval.setMinimum(-10000000)
        self.intval.setMaximum(10000000)
        self.intval.setObjectName(_fromUtf8("intval"))
        self.label_4 = QtGui.QLabel(assvardlg)
        self.label_4.setGeometry(QtCore.QRect(30, 120, 71, 17))
        self.label_4.setObjectName(_fromUtf8("label_4"))

        self.retranslateUi(assvardlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), assvardlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), assvardlg.reject)
        QtCore.QMetaObject.connectSlotsByName(assvardlg)
        assvardlg.setTabOrder(self.intval, self.istext)
        assvardlg.setTabOrder(self.istext, self.textval)
        assvardlg.setTabOrder(self.textval, self.buttonBox)

    def retranslateUi(self, assvardlg):
        assvardlg.setWindowTitle(QtGui.QApplication.translate("assvardlg", "Assign value to variable", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("assvardlg", "Assign var:", None, QtGui.QApplication.UnicodeUTF8))
        self.textval.setToolTip(QtGui.QApplication.translate("assvardlg", "Give the text value to be used if variable has to have this\n"
"", None, QtGui.QApplication.UnicodeUTF8))
        self.label_3.setText(QtGui.QApplication.translate("assvardlg", "Value", None, QtGui.QApplication.UnicodeUTF8))
        self.istext.setToolTip(QtGui.QApplication.translate("assvardlg", "Indicates variable has text value", None, QtGui.QApplication.UnicodeUTF8))
        self.istext.setText(QtGui.QApplication.translate("assvardlg", "Has Text Value", None, QtGui.QApplication.UnicodeUTF8))
        self.intval.setToolTip(QtGui.QApplication.translate("assvardlg", "This is the value for the variable if an integer", None, QtGui.QApplication.UnicodeUTF8))
        self.label_4.setText(QtGui.QApplication.translate("assvardlg", "Text value", None, QtGui.QApplication.UnicodeUTF8))

