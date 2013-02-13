# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'jenvdlg.ui'
#
# Created: Tue Nov 20 09:31:20 2012
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_jenvdlg(object):
    def setupUi(self, jenvdlg):
        jenvdlg.setObjectName(_fromUtf8("jenvdlg"))
        jenvdlg.resize(909, 274)
        self.buttonBox = QtGui.QDialogButtonBox(jenvdlg)
        self.buttonBox.setGeometry(QtCore.QRect(660, 220, 221, 32))
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.edjob = QtGui.QLabel(jenvdlg)
        self.edjob.setGeometry(QtCore.QRect(310, 50, 331, 17))
        self.edjob.setText(_fromUtf8(""))
        self.edjob.setObjectName(_fromUtf8("edjob"))
        self.label = QtGui.QLabel(jenvdlg)
        self.label.setGeometry(QtCore.QRect(40, 50, 201, 17))
        self.label.setObjectName(_fromUtf8("label"))
        self.label_2 = QtGui.QLabel(jenvdlg)
        self.label_2.setGeometry(QtCore.QRect(40, 110, 66, 17))
        self.label_2.setObjectName(_fromUtf8("label_2"))
        self.label_3 = QtGui.QLabel(jenvdlg)
        self.label_3.setGeometry(QtCore.QRect(40, 160, 66, 17))
        self.label_3.setObjectName(_fromUtf8("label_3"))
        self.ename = QtGui.QLineEdit(jenvdlg)
        self.ename.setGeometry(QtCore.QRect(130, 100, 281, 27))
        font = QtGui.QFont()
        font.setFamily(_fromUtf8("Courier New"))
        font.setPointSize(10)
        self.ename.setFont(font)
        self.ename.setObjectName(_fromUtf8("ename"))
        self.evalue = QtGui.QLineEdit(jenvdlg)
        self.evalue.setGeometry(QtCore.QRect(130, 160, 731, 27))
        font = QtGui.QFont()
        font.setFamily(_fromUtf8("Courier New"))
        self.evalue.setFont(font)
        self.evalue.setObjectName(_fromUtf8("evalue"))

        self.retranslateUi(jenvdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), jenvdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), jenvdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(jenvdlg)
        jenvdlg.setTabOrder(self.ename, self.evalue)
        jenvdlg.setTabOrder(self.evalue, self.buttonBox)

    def retranslateUi(self, jenvdlg):
        jenvdlg.setWindowTitle(QtGui.QApplication.translate("jenvdlg", "Edit environment variable", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("jenvdlg", "Environment variable for job", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("jenvdlg", "Name", None, QtGui.QApplication.UnicodeUTF8))
        self.label_3.setText(QtGui.QApplication.translate("jenvdlg", "Value", None, QtGui.QApplication.UnicodeUTF8))
        self.ename.setToolTip(QtGui.QApplication.translate("jenvdlg", "This is the name of the variable", None, QtGui.QApplication.UnicodeUTF8))
        self.evalue.setToolTip(QtGui.QApplication.translate("jenvdlg", "This is the value of the variable", None, QtGui.QApplication.UnicodeUTF8))

