# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'jenvsdlg.ui'
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

class Ui_jenvsdlg(object):
    def setupUi(self, jenvsdlg):
        jenvsdlg.setObjectName(_fromUtf8("jenvsdlg"))
        jenvsdlg.resize(823, 407)
        self.buttonBox = QtGui.QDialogButtonBox(jenvsdlg)
        self.buttonBox.setGeometry(QtCore.QRect(610, 360, 191, 32))
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.edjob = QtGui.QLabel(jenvsdlg)
        self.edjob.setGeometry(QtCore.QRect(130, 30, 331, 17))
        self.edjob.setText(_fromUtf8(""))
        self.edjob.setObjectName(_fromUtf8("edjob"))
        self.label = QtGui.QLabel(jenvsdlg)
        self.label.setGeometry(QtCore.QRect(30, 30, 91, 17))
        self.label.setObjectName(_fromUtf8("label"))
        self.envlist = QtGui.QListWidget(jenvsdlg)
        self.envlist.setGeometry(QtCore.QRect(30, 70, 771, 192))
        font = QtGui.QFont()
        font.setFamily(_fromUtf8("Courier New"))
        font.setPointSize(10)
        self.envlist.setFont(font)
        self.envlist.setDragDropMode(QtGui.QAbstractItemView.InternalMove)
        self.envlist.setObjectName(_fromUtf8("envlist"))
        self.newenv = QtGui.QPushButton(jenvsdlg)
        self.newenv.setGeometry(QtCore.QRect(30, 300, 98, 27))
        self.newenv.setObjectName(_fromUtf8("newenv"))
        self.delenv = QtGui.QPushButton(jenvsdlg)
        self.delenv.setGeometry(QtCore.QRect(700, 300, 98, 27))
        self.delenv.setObjectName(_fromUtf8("delenv"))

        self.retranslateUi(jenvsdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), jenvsdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), jenvsdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(jenvsdlg)
        jenvsdlg.setTabOrder(self.envlist, self.newenv)
        jenvsdlg.setTabOrder(self.newenv, self.delenv)
        jenvsdlg.setTabOrder(self.delenv, self.buttonBox)

    def retranslateUi(self, jenvsdlg):
        jenvsdlg.setWindowTitle(QtGui.QApplication.translate("jenvsdlg", "Environment Variable List", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("jenvsdlg", "Env for job:", None, QtGui.QApplication.UnicodeUTF8))
        self.envlist.setToolTip(QtGui.QApplication.translate("jenvsdlg", "These are the environment variables for the job.\n"
"\n"
"Drag and drop to re-order.\n"
"\n"
"Double-click to edit\n"
"\n"
"Click \"New\" to add a new environment variable.\n"
"\n"
"Select an item and press \"Delete\" to delete an environment variable.", None, QtGui.QApplication.UnicodeUTF8))
        self.newenv.setToolTip(QtGui.QApplication.translate("jenvsdlg", "Click this to add a new environment variable", None, QtGui.QApplication.UnicodeUTF8))
        self.newenv.setText(QtGui.QApplication.translate("jenvsdlg", "New", None, QtGui.QApplication.UnicodeUTF8))
        self.delenv.setToolTip(QtGui.QApplication.translate("jenvsdlg", "Press this to delete the currently-selected environment variable.", None, QtGui.QApplication.UnicodeUTF8))
        self.delenv.setText(QtGui.QApplication.translate("jenvsdlg", "Delete", None, QtGui.QApplication.UnicodeUTF8))

