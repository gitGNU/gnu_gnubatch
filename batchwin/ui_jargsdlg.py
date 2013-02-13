# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'jargsdlg.ui'
#
# Created: Mon Nov 19 08:57:55 2012
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_jargsdlg(object):
    def setupUi(self, jargsdlg):
        jargsdlg.setObjectName(_fromUtf8("jargsdlg"))
        jargsdlg.resize(508, 414)
        self.buttonBox = QtGui.QDialogButtonBox(jargsdlg)
        self.buttonBox.setGeometry(QtCore.QRect(290, 370, 201, 32))
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.edjob = QtGui.QLabel(jargsdlg)
        self.edjob.setGeometry(QtCore.QRect(150, 30, 331, 17))
        self.edjob.setText(_fromUtf8(""))
        self.edjob.setObjectName(_fromUtf8("edjob"))
        self.label = QtGui.QLabel(jargsdlg)
        self.label.setGeometry(QtCore.QRect(50, 30, 91, 17))
        self.label.setObjectName(_fromUtf8("label"))
        self.newarg = QtGui.QPushButton(jargsdlg)
        self.newarg.setGeometry(QtCore.QRect(50, 310, 98, 27))
        self.newarg.setObjectName(_fromUtf8("newarg"))
        self.delarg = QtGui.QPushButton(jargsdlg)
        self.delarg.setGeometry(QtCore.QRect(390, 310, 98, 27))
        self.delarg.setObjectName(_fromUtf8("delarg"))
        self.arglist = QtGui.QListWidget(jargsdlg)
        self.arglist.setGeometry(QtCore.QRect(50, 80, 441, 192))
        font = QtGui.QFont()
        font.setFamily(_fromUtf8("Courier New"))
        font.setPointSize(10)
        self.arglist.setFont(font)
        self.arglist.setDragDropMode(QtGui.QAbstractItemView.InternalMove)
        self.arglist.setObjectName(_fromUtf8("arglist"))

        self.retranslateUi(jargsdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), jargsdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), jargsdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(jargsdlg)
        jargsdlg.setTabOrder(self.arglist, self.newarg)
        jargsdlg.setTabOrder(self.newarg, self.delarg)
        jargsdlg.setTabOrder(self.delarg, self.buttonBox)

    def retranslateUi(self, jargsdlg):
        jargsdlg.setWindowTitle(QtGui.QApplication.translate("jargsdlg", "Job Argument List", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("jargsdlg", "Args for job:", None, QtGui.QApplication.UnicodeUTF8))
        self.newarg.setToolTip(QtGui.QApplication.translate("jargsdlg", "Click this to add a new argument", None, QtGui.QApplication.UnicodeUTF8))
        self.newarg.setText(QtGui.QApplication.translate("jargsdlg", "New", None, QtGui.QApplication.UnicodeUTF8))
        self.delarg.setToolTip(QtGui.QApplication.translate("jargsdlg", "Press this to delete the currently-selected argument", None, QtGui.QApplication.UnicodeUTF8))
        self.delarg.setText(QtGui.QApplication.translate("jargsdlg", "Delete", None, QtGui.QApplication.UnicodeUTF8))
        self.arglist.setToolTip(QtGui.QApplication.translate("jargsdlg", "These are the arguments for the job.\n"
"\n"
"Drag and drop to re-order.\n"
"\n"
"Double-click to edit\n"
"\n"
"Click \"New\" to add a new argument.\n"
"\n"
"Select an item and press \"Delete\" to delete an argument.", None, QtGui.QApplication.UnicodeUTF8))

