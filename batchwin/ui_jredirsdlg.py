# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'jredirsdlg.ui'
#
# Created: Tue Nov 20 09:31:19 2012
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_jredirsdlg(object):
    def setupUi(self, jredirsdlg):
        jredirsdlg.setObjectName(_fromUtf8("jredirsdlg"))
        jredirsdlg.resize(644, 451)
        self.buttonBox = QtGui.QDialogButtonBox(jredirsdlg)
        self.buttonBox.setGeometry(QtCore.QRect(410, 400, 201, 32))
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.edjob = QtGui.QLabel(jredirsdlg)
        self.edjob.setGeometry(QtCore.QRect(120, 20, 331, 17))
        self.edjob.setText(_fromUtf8(""))
        self.edjob.setObjectName(_fromUtf8("edjob"))
        self.label = QtGui.QLabel(jredirsdlg)
        self.label.setGeometry(QtCore.QRect(20, 20, 91, 17))
        self.label.setObjectName(_fromUtf8("label"))
        self.redirlist = QtGui.QListWidget(jredirsdlg)
        self.redirlist.setGeometry(QtCore.QRect(20, 70, 591, 221))
        font = QtGui.QFont()
        font.setFamily(_fromUtf8("Courier New"))
        font.setPointSize(10)
        self.redirlist.setFont(font)
        self.redirlist.setDragDropMode(QtGui.QAbstractItemView.InternalMove)
        self.redirlist.setObjectName(_fromUtf8("redirlist"))
        self.newredir = QtGui.QPushButton(jredirsdlg)
        self.newredir.setGeometry(QtCore.QRect(20, 330, 98, 27))
        self.newredir.setObjectName(_fromUtf8("newredir"))
        self.delredir = QtGui.QPushButton(jredirsdlg)
        self.delredir.setGeometry(QtCore.QRect(510, 330, 98, 27))
        self.delredir.setObjectName(_fromUtf8("delredir"))

        self.retranslateUi(jredirsdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), jredirsdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), jredirsdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(jredirsdlg)
        jredirsdlg.setTabOrder(self.redirlist, self.newredir)
        jredirsdlg.setTabOrder(self.newredir, self.delredir)
        jredirsdlg.setTabOrder(self.delredir, self.buttonBox)

    def retranslateUi(self, jredirsdlg):
        jredirsdlg.setWindowTitle(QtGui.QApplication.translate("jredirsdlg", "Job I/O Redirections", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("jredirsdlg", "Redirs for job:", None, QtGui.QApplication.UnicodeUTF8))
        self.redirlist.setToolTip(QtGui.QApplication.translate("jredirsdlg", "These are the IO Redirections for the job.\n"
"\n"
"Drag and drop to re-order.\n"
"\n"
"Double-click to edit\n"
"\n"
"Click \"New\" to add a new redirections.\n"
"\n"
"Select an item and press \"Delete\" to delete an redirection.", None, QtGui.QApplication.UnicodeUTF8))
        self.newredir.setToolTip(QtGui.QApplication.translate("jredirsdlg", "Click this to add a new redirection", None, QtGui.QApplication.UnicodeUTF8))
        self.newredir.setText(QtGui.QApplication.translate("jredirsdlg", "New", None, QtGui.QApplication.UnicodeUTF8))
        self.delredir.setToolTip(QtGui.QApplication.translate("jredirsdlg", "Press this to delete the currently-selected redirection", None, QtGui.QApplication.UnicodeUTF8))
        self.delredir.setText(QtGui.QApplication.translate("jredirsdlg", "Delete", None, QtGui.QApplication.UnicodeUTF8))

