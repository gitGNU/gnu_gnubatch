# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'jobviewdlg.ui'
#
# Created: Sat Oct 27 12:21:55 2012
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_jobviewdlg(object):
    def setupUi(self, jobviewdlg):
        jobviewdlg.setObjectName(_fromUtf8("jobviewdlg"))
        jobviewdlg.resize(638, 526)
        icon = QtGui.QIcon()
        icon.addPixmap(QtGui.QPixmap(_fromUtf8("btqwdoc.png")), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        jobviewdlg.setWindowIcon(icon)
        jobviewdlg.setAutoFillBackground(False)
        jobviewdlg.setSizeGripEnabled(True)
        self.verticalLayout_2 = QtGui.QVBoxLayout(jobviewdlg)
        self.verticalLayout_2.setObjectName(_fromUtf8("verticalLayout_2"))
        self.verticalLayout = QtGui.QVBoxLayout()
        self.verticalLayout.setSizeConstraint(QtGui.QLayout.SetDefaultConstraint)
        self.verticalLayout.setObjectName(_fromUtf8("verticalLayout"))
        self.horizontalLayout = QtGui.QHBoxLayout()
        self.horizontalLayout.setObjectName(_fromUtf8("horizontalLayout"))
        self.label = QtGui.QLabel(jobviewdlg)
        self.label.setObjectName(_fromUtf8("label"))
        self.horizontalLayout.addWidget(self.label)
        self.jobdescr = QtGui.QLineEdit(jobviewdlg)
        self.jobdescr.setReadOnly(True)
        self.jobdescr.setObjectName(_fromUtf8("jobdescr"))
        self.horizontalLayout.addWidget(self.jobdescr)
        self.verticalLayout.addLayout(self.horizontalLayout)
        self.jobtext = QtGui.QPlainTextEdit(jobviewdlg)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.jobtext.sizePolicy().hasHeightForWidth())
        self.jobtext.setSizePolicy(sizePolicy)
        font = QtGui.QFont()
        font.setFamily(_fromUtf8("Courier 10 Pitch"))
        self.jobtext.setFont(font)
        self.jobtext.setLineWrapMode(QtGui.QPlainTextEdit.NoWrap)
        self.jobtext.setReadOnly(True)
        self.jobtext.setObjectName(_fromUtf8("jobtext"))
        self.verticalLayout.addWidget(self.jobtext)
        self.buttonBox = QtGui.QDialogButtonBox(jobviewdlg)
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Close)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.verticalLayout.addWidget(self.buttonBox)
        self.verticalLayout_2.addLayout(self.verticalLayout)

        self.retranslateUi(jobviewdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), jobviewdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), jobviewdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(jobviewdlg)
        jobviewdlg.setTabOrder(self.jobdescr, self.jobtext)
        jobviewdlg.setTabOrder(self.jobtext, self.buttonBox)

    def retranslateUi(self, jobviewdlg):
        jobviewdlg.setWindowTitle(QtGui.QApplication.translate("jobviewdlg", "Job View", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("jobviewdlg", "Viewing job", None, QtGui.QApplication.UnicodeUTF8))

