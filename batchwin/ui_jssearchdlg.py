# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'jssearchdlg.ui'
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

class Ui_jssearchdlg(object):
    def setupUi(self, jssearchdlg):
        jssearchdlg.setObjectName(_fromUtf8("jssearchdlg"))
        jssearchdlg.resize(400, 234)
        self.buttonBox = QtGui.QDialogButtonBox(jssearchdlg)
        self.buttonBox.setGeometry(QtCore.QRect(310, 10, 81, 101))
        self.buttonBox.setOrientation(QtCore.Qt.Vertical)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.groupBox = QtGui.QGroupBox(jssearchdlg)
        self.groupBox.setGeometry(QtCore.QRect(30, 130, 120, 80))
        self.groupBox.setObjectName(_fromUtf8("groupBox"))
        self.sforward = QtGui.QRadioButton(self.groupBox)
        self.sforward.setGeometry(QtCore.QRect(10, 30, 108, 23))
        self.sforward.setChecked(True)
        self.sforward.setObjectName(_fromUtf8("sforward"))
        self.sbackward = QtGui.QRadioButton(self.groupBox)
        self.sbackward.setGeometry(QtCore.QRect(10, 60, 108, 21))
        self.sbackward.setObjectName(_fromUtf8("sbackward"))
        self.wrapround = QtGui.QCheckBox(jssearchdlg)
        self.wrapround.setGeometry(QtCore.QRect(210, 160, 111, 23))
        self.wrapround.setObjectName(_fromUtf8("wrapround"))
        self.label = QtGui.QLabel(jssearchdlg)
        self.label.setGeometry(QtCore.QRect(10, 30, 81, 18))
        self.label.setObjectName(_fromUtf8("label"))
        self.searchstring = QtGui.QLineEdit(jssearchdlg)
        self.searchstring.setGeometry(QtCore.QRect(10, 60, 251, 26))
        self.searchstring.setObjectName(_fromUtf8("searchstring"))
        self.igncase = QtGui.QCheckBox(jssearchdlg)
        self.igncase.setGeometry(QtCore.QRect(210, 190, 121, 23))
        self.igncase.setChecked(True)
        self.igncase.setObjectName(_fromUtf8("igncase"))
        self.layoutWidget = QtGui.QWidget(jssearchdlg)
        self.layoutWidget.setGeometry(QtCore.QRect(0, 0, 2, 2))
        self.layoutWidget.setObjectName(_fromUtf8("layoutWidget"))
        self.verticalLayout = QtGui.QVBoxLayout(self.layoutWidget)
        self.verticalLayout.setMargin(0)
        self.verticalLayout.setObjectName(_fromUtf8("verticalLayout"))
        self.layoutWidget1 = QtGui.QWidget(jssearchdlg)
        self.layoutWidget1.setGeometry(QtCore.QRect(0, 0, 2, 2))
        self.layoutWidget1.setObjectName(_fromUtf8("layoutWidget1"))
        self.verticalLayout_2 = QtGui.QVBoxLayout(self.layoutWidget1)
        self.verticalLayout_2.setMargin(0)
        self.verticalLayout_2.setObjectName(_fromUtf8("verticalLayout_2"))

        self.retranslateUi(jssearchdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), jssearchdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), jssearchdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(jssearchdlg)
        jssearchdlg.setTabOrder(self.searchstring, self.sforward)
        jssearchdlg.setTabOrder(self.sforward, self.sbackward)
        jssearchdlg.setTabOrder(self.sbackward, self.wrapround)
        jssearchdlg.setTabOrder(self.wrapround, self.igncase)
        jssearchdlg.setTabOrder(self.igncase, self.buttonBox)

    def retranslateUi(self, jssearchdlg):
        jssearchdlg.setWindowTitle(QtGui.QApplication.translate("jssearchdlg", "Search job strings", None, QtGui.QApplication.UnicodeUTF8))
        self.groupBox.setTitle(QtGui.QApplication.translate("jssearchdlg", "Direction", None, QtGui.QApplication.UnicodeUTF8))
        self.sforward.setToolTip(QtGui.QApplication.translate("jssearchdlg", "Select to search forwards", None, QtGui.QApplication.UnicodeUTF8))
        self.sforward.setText(QtGui.QApplication.translate("jssearchdlg", "Forwards", None, QtGui.QApplication.UnicodeUTF8))
        self.sbackward.setToolTip(QtGui.QApplication.translate("jssearchdlg", "Select to search backwards", None, QtGui.QApplication.UnicodeUTF8))
        self.sbackward.setText(QtGui.QApplication.translate("jssearchdlg", "Backwards", None, QtGui.QApplication.UnicodeUTF8))
        self.wrapround.setToolTip(QtGui.QApplication.translate("jssearchdlg", "Select to wrap search round", None, QtGui.QApplication.UnicodeUTF8))
        self.wrapround.setText(QtGui.QApplication.translate("jssearchdlg", "Wrap round", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("jssearchdlg", "Search for", None, QtGui.QApplication.UnicodeUTF8))
        self.searchstring.setToolTip(QtGui.QApplication.translate("jssearchdlg", "This is the string to search for with . for any character", None, QtGui.QApplication.UnicodeUTF8))
        self.igncase.setToolTip(QtGui.QApplication.translate("jssearchdlg", "Select this to ignore case differences on letters", None, QtGui.QApplication.UnicodeUTF8))
        self.igncase.setText(QtGui.QApplication.translate("jssearchdlg", "Ignore Case", None, QtGui.QApplication.UnicodeUTF8))

