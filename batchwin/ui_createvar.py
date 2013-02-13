# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'createvar.ui'
#
# Created: Sat Nov 24 18:45:50 2012
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_crevardlg(object):
    def setupUi(self, crevardlg):
        crevardlg.setObjectName(_fromUtf8("crevardlg"))
        crevardlg.resize(561, 338)
        self.buttonBox = QtGui.QDialogButtonBox(crevardlg)
        self.buttonBox.setGeometry(QtCore.QRect(340, 290, 191, 32))
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.label = QtGui.QLabel(crevardlg)
        self.label.setGeometry(QtCore.QRect(50, 70, 51, 17))
        self.label.setObjectName(_fromUtf8("label"))
        self.varname = QtGui.QLineEdit(crevardlg)
        self.varname.setGeometry(QtCore.QRect(150, 70, 221, 27))
        self.varname.setMaxLength(19)
        self.varname.setObjectName(_fromUtf8("varname"))
        self.label_2 = QtGui.QLabel(crevardlg)
        self.label_2.setGeometry(QtCore.QRect(50, 110, 91, 17))
        self.label_2.setObjectName(_fromUtf8("label_2"))
        self.comment = QtGui.QLineEdit(crevardlg)
        self.comment.setGeometry(QtCore.QRect(150, 110, 381, 27))
        self.comment.setMaxLength(41)
        self.comment.setObjectName(_fromUtf8("comment"))
        self.label_3 = QtGui.QLabel(crevardlg)
        self.label_3.setGeometry(QtCore.QRect(50, 150, 51, 17))
        self.label_3.setObjectName(_fromUtf8("label_3"))
        self.intval = QtGui.QSpinBox(crevardlg)
        self.intval.setGeometry(QtCore.QRect(150, 150, 101, 27))
        self.intval.setMinimum(-10000000)
        self.intval.setMaximum(10000000)
        self.intval.setObjectName(_fromUtf8("intval"))
        self.istext = QtGui.QCheckBox(crevardlg)
        self.istext.setGeometry(QtCore.QRect(310, 150, 131, 22))
        self.istext.setObjectName(_fromUtf8("istext"))
        self.textval = QtGui.QLineEdit(crevardlg)
        self.textval.setEnabled(False)
        self.textval.setGeometry(QtCore.QRect(150, 190, 347, 28))
        font = QtGui.QFont()
        font.setFamily(_fromUtf8("Courier New"))
        font.setBold(False)
        font.setItalic(False)
        font.setWeight(50)
        self.textval.setFont(font)
        self.textval.setMaxLength(49)
        self.textval.setObjectName(_fromUtf8("textval"))
        self.label_4 = QtGui.QLabel(crevardlg)
        self.label_4.setGeometry(QtCore.QRect(50, 200, 71, 17))
        self.label_4.setObjectName(_fromUtf8("label_4"))
        self.exporttype = QtGui.QComboBox(crevardlg)
        self.exporttype.setGeometry(QtCore.QRect(50, 250, 191, 27))
        self.exporttype.setObjectName(_fromUtf8("exporttype"))
        self.exporttype.addItem(_fromUtf8(""))
        self.exporttype.addItem(_fromUtf8(""))
        self.exporttype.addItem(_fromUtf8(""))
        self.label_5 = QtGui.QLabel(crevardlg)
        self.label_5.setGeometry(QtCore.QRect(50, 30, 111, 17))
        self.label_5.setObjectName(_fromUtf8("label_5"))
        self.crehost = QtGui.QComboBox(crevardlg)
        self.crehost.setGeometry(QtCore.QRect(170, 20, 181, 27))
        self.crehost.setObjectName(_fromUtf8("crehost"))

        self.retranslateUi(crevardlg)
        self.exporttype.setCurrentIndex(1)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), crevardlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), crevardlg.reject)
        QtCore.QMetaObject.connectSlotsByName(crevardlg)
        crevardlg.setTabOrder(self.crehost, self.varname)
        crevardlg.setTabOrder(self.varname, self.comment)
        crevardlg.setTabOrder(self.comment, self.intval)
        crevardlg.setTabOrder(self.intval, self.istext)
        crevardlg.setTabOrder(self.istext, self.textval)
        crevardlg.setTabOrder(self.textval, self.exporttype)
        crevardlg.setTabOrder(self.exporttype, self.buttonBox)

    def retranslateUi(self, crevardlg):
        crevardlg.setWindowTitle(QtGui.QApplication.translate("crevardlg", "Create Variable", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("crevardlg", "Name", None, QtGui.QApplication.UnicodeUTF8))
        self.varname.setToolTip(QtGui.QApplication.translate("crevardlg", "This is the name of the variable", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("crevardlg", "Description", None, QtGui.QApplication.UnicodeUTF8))
        self.comment.setToolTip(QtGui.QApplication.translate("crevardlg", "This is a description or comment about the variable", None, QtGui.QApplication.UnicodeUTF8))
        self.label_3.setText(QtGui.QApplication.translate("crevardlg", "Value", None, QtGui.QApplication.UnicodeUTF8))
        self.intval.setToolTip(QtGui.QApplication.translate("crevardlg", "This is the value for the variable if an integer", None, QtGui.QApplication.UnicodeUTF8))
        self.istext.setToolTip(QtGui.QApplication.translate("crevardlg", "Indicates variable has text value", None, QtGui.QApplication.UnicodeUTF8))
        self.istext.setText(QtGui.QApplication.translate("crevardlg", "Has Text Value", None, QtGui.QApplication.UnicodeUTF8))
        self.textval.setToolTip(QtGui.QApplication.translate("crevardlg", "Give the text value to be used if variable has to have this\n"
"", None, QtGui.QApplication.UnicodeUTF8))
        self.label_4.setText(QtGui.QApplication.translate("crevardlg", "Text value", None, QtGui.QApplication.UnicodeUTF8))
        self.exporttype.setToolTip(QtGui.QApplication.translate("crevardlg", "Set export marker on variable\n"
"\n"
"Local only means it\'s only visible and usable on the server in question.\n"
"\n"
"Exported means it can be visible on all hosts\n"
"\n"
"Clustered (this is going to be phased out) means that the version on the\n"
"server is always used.", None, QtGui.QApplication.UnicodeUTF8))
        self.exporttype.setItemText(0, QtGui.QApplication.translate("crevardlg", "Local Only", None, QtGui.QApplication.UnicodeUTF8))
        self.exporttype.setItemText(1, QtGui.QApplication.translate("crevardlg", "Exported", None, QtGui.QApplication.UnicodeUTF8))
        self.exporttype.setItemText(2, QtGui.QApplication.translate("crevardlg", "Clustered", None, QtGui.QApplication.UnicodeUTF8))
        self.label_5.setText(QtGui.QApplication.translate("crevardlg", "Create on host", None, QtGui.QApplication.UnicodeUTF8))
        self.crehost.setToolTip(QtGui.QApplication.translate("crevardlg", "This is the server on which the variable is created", None, QtGui.QApplication.UnicodeUTF8))

