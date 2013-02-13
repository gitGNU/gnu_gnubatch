# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'progopts.ui'
#
# Created: Tue Aug 14 11:40:54 2012
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_Progopts(object):
    def setupUi(self, Progopts):
        Progopts.setObjectName(_fromUtf8("Progopts"))
        Progopts.resize(400, 233)
        self.buttonBox = QtGui.QDialogButtonBox(Progopts)
        self.buttonBox.setGeometry(QtCore.QRect(290, 20, 81, 111))
        self.buttonBox.setOrientation(QtCore.Qt.Vertical)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.confdel = QtGui.QGroupBox(Progopts)
        self.confdel.setGeometry(QtCore.QRect(20, 10, 231, 111))
        self.confdel.setObjectName(_fromUtf8("confdel"))
        self.layoutWidget = QtGui.QWidget(self.confdel)
        self.layoutWidget.setGeometry(QtCore.QRect(20, 29, 191, 61))
        self.layoutWidget.setObjectName(_fromUtf8("layoutWidget"))
        self.verticalLayout = QtGui.QVBoxLayout(self.layoutWidget)
        self.verticalLayout.setMargin(0)
        self.verticalLayout.setObjectName(_fromUtf8("verticalLayout"))
        self.confNever = QtGui.QRadioButton(self.layoutWidget)
        self.confNever.setObjectName(_fromUtf8("confNever"))
        self.verticalLayout.addWidget(self.confNever)
        self.confAlways = QtGui.QRadioButton(self.layoutWidget)
        self.confAlways.setChecked(True)
        self.confAlways.setObjectName(_fromUtf8("confAlways"))
        self.verticalLayout.addWidget(self.confAlways)
        self.layoutWidget_2 = QtGui.QWidget(Progopts)
        self.layoutWidget_2.setGeometry(QtCore.QRect(40, 180, 241, 41))
        self.layoutWidget_2.setObjectName(_fromUtf8("layoutWidget_2"))
        self.horizontalLayout_2 = QtGui.QHBoxLayout(self.layoutWidget_2)
        self.horizontalLayout_2.setMargin(0)
        self.horizontalLayout_2.setObjectName(_fromUtf8("horizontalLayout_2"))
        self.label_2 = QtGui.QLabel(self.layoutWidget_2)
        self.label_2.setObjectName(_fromUtf8("label_2"))
        self.horizontalLayout_2.addWidget(self.label_2)
        self.udpwaittime = QtGui.QSpinBox(self.layoutWidget_2)
        self.udpwaittime.setMinimum(1)
        self.udpwaittime.setMaximum(32767)
        self.udpwaittime.setSingleStep(50)
        self.udpwaittime.setProperty("value", 750)
        self.udpwaittime.setObjectName(_fromUtf8("udpwaittime"))
        self.horizontalLayout_2.addWidget(self.udpwaittime)
        self.unqueuebin = QtGui.QCheckBox(Progopts)
        self.unqueuebin.setGeometry(QtCore.QRect(40, 130, 151, 22))
        self.unqueuebin.setObjectName(_fromUtf8("unqueuebin"))
        self.label_2.setBuddy(self.udpwaittime)

        self.retranslateUi(Progopts)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), Progopts.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), Progopts.reject)
        QtCore.QMetaObject.connectSlotsByName(Progopts)
        Progopts.setTabOrder(self.confNever, self.confAlways)
        Progopts.setTabOrder(self.confAlways, self.unqueuebin)
        Progopts.setTabOrder(self.unqueuebin, self.udpwaittime)
        Progopts.setTabOrder(self.udpwaittime, self.buttonBox)

    def retranslateUi(self, Progopts):
        Progopts.setWindowTitle(QtGui.QApplication.translate("Progopts", "Program Options", None, QtGui.QApplication.UnicodeUTF8))
        self.confdel.setTitle(QtGui.QApplication.translate("Progopts", "Delete Job Confirmation", None, QtGui.QApplication.UnicodeUTF8))
        self.confNever.setToolTip(QtGui.QApplication.translate("Progopts", "Do not ask confirmation before deleting a job - just delete", None, QtGui.QApplication.UnicodeUTF8))
        self.confNever.setText(QtGui.QApplication.translate("Progopts", "&Never - just delete", None, QtGui.QApplication.UnicodeUTF8))
        self.confAlways.setToolTip(QtGui.QApplication.translate("Progopts", "Always ask confirmation before deleting any job", None, QtGui.QApplication.UnicodeUTF8))
        self.confAlways.setText(QtGui.QApplication.translate("Progopts", "&Ask first", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("Progopts", "&UDP wait time", None, QtGui.QApplication.UnicodeUTF8))
        self.udpwaittime.setToolTip(QtGui.QApplication.translate("Progopts", "Set wait time for UDP calls (login/logout)", None, QtGui.QApplication.UnicodeUTF8))
        self.udpwaittime.setSuffix(QtGui.QApplication.translate("Progopts", " ms", None, QtGui.QApplication.UnicodeUTF8))
        self.unqueuebin.setToolTip(QtGui.QApplication.translate("Progopts", "Unqueue job in binary mode.\n"
"(I.e. on Windows clients don\'t convert line endings to CRLF).", None, QtGui.QApplication.UnicodeUTF8))
        self.unqueuebin.setText(QtGui.QApplication.translate("Progopts", "Unqueue as binary", None, QtGui.QApplication.UnicodeUTF8))

