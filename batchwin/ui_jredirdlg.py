# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'jredirdlg.ui'
#
# Created: Tue Nov 20 19:58:14 2012
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_jredirdlg(object):
    def setupUi(self, jredirdlg):
        jredirdlg.setObjectName(_fromUtf8("jredirdlg"))
        jredirdlg.resize(568, 300)
        self.buttonBox = QtGui.QDialogButtonBox(jredirdlg)
        self.buttonBox.setGeometry(QtCore.QRect(290, 260, 211, 32))
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.label = QtGui.QLabel(jredirdlg)
        self.label.setGeometry(QtCore.QRect(30, 20, 141, 17))
        self.label.setObjectName(_fromUtf8("label"))
        self.edjob = QtGui.QLabel(jredirdlg)
        self.edjob.setGeometry(QtCore.QRect(170, 20, 331, 17))
        self.edjob.setText(_fromUtf8(""))
        self.edjob.setObjectName(_fromUtf8("edjob"))
        self.label_2 = QtGui.QLabel(jredirdlg)
        self.label_2.setGeometry(QtCore.QRect(30, 60, 101, 17))
        self.label_2.setObjectName(_fromUtf8("label_2"))
        self.fd = QtGui.QSpinBox(jredirdlg)
        self.fd.setGeometry(QtCore.QRect(140, 60, 81, 27))
        self.fd.setMaximum(1023)
        self.fd.setSingleStep(1)
        self.fd.setProperty("value", 100)
        self.fd.setObjectName(_fromUtf8("fd"))
        self.expl = QtGui.QLabel(jredirdlg)
        self.expl.setGeometry(QtCore.QRect(300, 60, 181, 17))
        self.expl.setText(_fromUtf8(""))
        self.expl.setObjectName(_fromUtf8("expl"))
        self.label_3 = QtGui.QLabel(jredirdlg)
        self.label_3.setGeometry(QtCore.QRect(30, 160, 91, 17))
        self.label_3.setObjectName(_fromUtf8("label_3"))
        self.filename = QtGui.QLineEdit(jredirdlg)
        self.filename.setGeometry(QtCore.QRect(140, 160, 391, 27))
        self.filename.setObjectName(_fromUtf8("filename"))
        self.action = QtGui.QComboBox(jredirdlg)
        self.action.setGeometry(QtCore.QRect(140, 110, 101, 27))
        self.action.setObjectName(_fromUtf8("action"))
        self.action.addItem(_fromUtf8(""))
        self.action.addItem(_fromUtf8(""))
        self.action.addItem(_fromUtf8(""))
        self.action.addItem(_fromUtf8(""))
        self.action.addItem(_fromUtf8(""))
        self.action.addItem(_fromUtf8(""))
        self.action.addItem(_fromUtf8(""))
        self.action.addItem(_fromUtf8(""))
        self.action.addItem(_fromUtf8(""))
        self.label_4 = QtGui.QLabel(jredirdlg)
        self.label_4.setGeometry(QtCore.QRect(30, 110, 66, 17))
        self.label_4.setObjectName(_fromUtf8("label_4"))
        self.label_5 = QtGui.QLabel(jredirdlg)
        self.label_5.setGeometry(QtCore.QRect(30, 210, 91, 17))
        self.label_5.setObjectName(_fromUtf8("label_5"))
        self.fd2 = QtGui.QSpinBox(jredirdlg)
        self.fd2.setGeometry(QtCore.QRect(140, 210, 81, 27))
        self.fd2.setMaximum(1023)
        self.fd2.setObjectName(_fromUtf8("fd2"))

        self.retranslateUi(jredirdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), jredirdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), jredirdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(jredirdlg)
        jredirdlg.setTabOrder(self.fd, self.action)
        jredirdlg.setTabOrder(self.action, self.filename)
        jredirdlg.setTabOrder(self.filename, self.fd2)
        jredirdlg.setTabOrder(self.fd2, self.buttonBox)

    def retranslateUi(self, jredirdlg):
        jredirdlg.setWindowTitle(QtGui.QApplication.translate("jredirdlg", "Job redirection", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("jredirdlg", "Redirection for job:", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("jredirdlg", "FD Applied to", None, QtGui.QApplication.UnicodeUTF8))
        self.fd.setToolTip(QtGui.QApplication.translate("jredirdlg", "This is the file descriptor which the redirection applies to", None, QtGui.QApplication.UnicodeUTF8))
        self.label_3.setText(QtGui.QApplication.translate("jredirdlg", "File/program", None, QtGui.QApplication.UnicodeUTF8))
        self.filename.setToolTip(QtGui.QApplication.translate("jredirdlg", "This is the file name to which the file redirections apply.\n"
"\n"
"For \"pipe to\" and \"pipe from\" operations, this gives the program\n"
"name.", None, QtGui.QApplication.UnicodeUTF8))
        self.action.setToolTip(QtGui.QApplication.translate("jredirdlg", "These are the possible actions of the redirection", None, QtGui.QApplication.UnicodeUTF8))
        self.action.setItemText(0, QtGui.QApplication.translate("jredirdlg", "Read", None, QtGui.QApplication.UnicodeUTF8))
        self.action.setItemText(1, QtGui.QApplication.translate("jredirdlg", "Write", None, QtGui.QApplication.UnicodeUTF8))
        self.action.setItemText(2, QtGui.QApplication.translate("jredirdlg", "Append", None, QtGui.QApplication.UnicodeUTF8))
        self.action.setItemText(3, QtGui.QApplication.translate("jredirdlg", "Read / Write", None, QtGui.QApplication.UnicodeUTF8))
        self.action.setItemText(4, QtGui.QApplication.translate("jredirdlg", "Read / Append", None, QtGui.QApplication.UnicodeUTF8))
        self.action.setItemText(5, QtGui.QApplication.translate("jredirdlg", "Pipe to", None, QtGui.QApplication.UnicodeUTF8))
        self.action.setItemText(6, QtGui.QApplication.translate("jredirdlg", "Pipe from", None, QtGui.QApplication.UnicodeUTF8))
        self.action.setItemText(7, QtGui.QApplication.translate("jredirdlg", "Close", None, QtGui.QApplication.UnicodeUTF8))
        self.action.setItemText(8, QtGui.QApplication.translate("jredirdlg", "Dup FD", None, QtGui.QApplication.UnicodeUTF8))
        self.label_4.setText(QtGui.QApplication.translate("jredirdlg", "Action", None, QtGui.QApplication.UnicodeUTF8))
        self.label_5.setText(QtGui.QApplication.translate("jredirdlg", "FD dup from", None, QtGui.QApplication.UnicodeUTF8))
        self.fd2.setToolTip(QtGui.QApplication.translate("jredirdlg", "This is the second file descriptior from which the first one\n"
"is duplicated, if applicable.", None, QtGui.QApplication.UnicodeUTF8))

