# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'jobeditdlg.ui'
#
# Created: Tue Jan 22 17:58:40 2013
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_jobeditdlg(object):
    def setupUi(self, jobeditdlg):
        jobeditdlg.setObjectName(_fromUtf8("jobeditdlg"))
        jobeditdlg.resize(877, 625)
        self.buttonBox = QtGui.QDialogButtonBox(jobeditdlg)
        self.buttonBox.setGeometry(QtCore.QRect(620, 580, 201, 32))
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.widget = QtGui.QWidget(jobeditdlg)
        self.widget.setGeometry(QtCore.QRect(11, 11, 851, 561))
        self.widget.setObjectName(_fromUtf8("widget"))
        self.gridLayout = QtGui.QGridLayout(self.widget)
        self.gridLayout.setMargin(0)
        self.gridLayout.setObjectName(_fromUtf8("gridLayout"))
        self.label = QtGui.QLabel(self.widget)
        self.label.setObjectName(_fromUtf8("label"))
        self.gridLayout.addWidget(self.label, 0, 0, 1, 1)
        self.jobdescr = QtGui.QLineEdit(self.widget)
        self.jobdescr.setReadOnly(True)
        self.jobdescr.setObjectName(_fromUtf8("jobdescr"))
        self.gridLayout.addWidget(self.jobdescr, 0, 1, 1, 1)
        self.jobtext = QtGui.QPlainTextEdit(self.widget)
        font = QtGui.QFont()
        font.setFamily(_fromUtf8("Courier New"))
        font.setPointSize(12)
        self.jobtext.setFont(font)
        self.jobtext.setObjectName(_fromUtf8("jobtext"))
        self.gridLayout.addWidget(self.jobtext, 1, 0, 1, 2)

        self.retranslateUi(jobeditdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), jobeditdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), jobeditdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(jobeditdlg)
        jobeditdlg.setTabOrder(self.jobdescr, self.jobtext)
        jobeditdlg.setTabOrder(self.jobtext, self.buttonBox)

    def retranslateUi(self, jobeditdlg):
        jobeditdlg.setWindowTitle(QtGui.QApplication.translate("jobeditdlg", "Edit job script", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("jobeditdlg", "Editing script for job", None, QtGui.QApplication.UnicodeUTF8))
        self.jobtext.setToolTip(QtGui.QApplication.translate("jobeditdlg", "This is the script of the job for the given command interpreter", None, QtGui.QApplication.UnicodeUTF8))

