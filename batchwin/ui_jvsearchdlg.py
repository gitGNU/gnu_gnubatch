# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'jvsearchdlg.ui'
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

class Ui_jvsearchdlg(object):
    def setupUi(self, jvsearchdlg):
        jvsearchdlg.setObjectName(_fromUtf8("jvsearchdlg"))
        jvsearchdlg.resize(519, 246)
        self.buttonBox = QtGui.QDialogButtonBox(jvsearchdlg)
        self.buttonBox.setGeometry(QtCore.QRect(420, 20, 81, 91))
        self.buttonBox.setOrientation(QtCore.Qt.Vertical)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.searchstring = QtGui.QLineEdit(jvsearchdlg)
        self.searchstring.setGeometry(QtCore.QRect(130, 30, 251, 26))
        self.searchstring.setObjectName(_fromUtf8("searchstring"))
        self.label = QtGui.QLabel(jvsearchdlg)
        self.label.setGeometry(QtCore.QRect(30, 30, 81, 18))
        self.label.setObjectName(_fromUtf8("label"))
        self.groupBox = QtGui.QGroupBox(jvsearchdlg)
        self.groupBox.setGeometry(QtCore.QRect(40, 70, 120, 80))
        self.groupBox.setObjectName(_fromUtf8("groupBox"))
        self.sforward = QtGui.QRadioButton(self.groupBox)
        self.sforward.setGeometry(QtCore.QRect(10, 30, 108, 23))
        self.sforward.setChecked(True)
        self.sforward.setObjectName(_fromUtf8("sforward"))
        self.sbackward = QtGui.QRadioButton(self.groupBox)
        self.sbackward.setGeometry(QtCore.QRect(10, 60, 108, 21))
        self.sbackward.setObjectName(_fromUtf8("sbackward"))
        self.wrapround = QtGui.QCheckBox(jvsearchdlg)
        self.wrapround.setGeometry(QtCore.QRect(50, 170, 111, 23))
        self.wrapround.setObjectName(_fromUtf8("wrapround"))
        self.igncase = QtGui.QCheckBox(jvsearchdlg)
        self.igncase.setGeometry(QtCore.QRect(50, 200, 121, 23))
        self.igncase.setChecked(True)
        self.igncase.setObjectName(_fromUtf8("igncase"))
        self.splitter = QtGui.QSplitter(jvsearchdlg)
        self.splitter.setGeometry(QtCore.QRect(240, 80, 94, 139))
        self.splitter.setOrientation(QtCore.Qt.Vertical)
        self.splitter.setObjectName(_fromUtf8("splitter"))
        self.suser = QtGui.QCheckBox(self.splitter)
        self.suser.setObjectName(_fromUtf8("suser"))
        self.stitle = QtGui.QCheckBox(self.splitter)
        self.stitle.setObjectName(_fromUtf8("stitle"))
        self.sname = QtGui.QCheckBox(self.splitter)
        self.sname.setObjectName(_fromUtf8("sname"))
        self.scomment = QtGui.QCheckBox(self.splitter)
        self.scomment.setObjectName(_fromUtf8("scomment"))
        self.svalue = QtGui.QCheckBox(self.splitter)
        self.svalue.setObjectName(_fromUtf8("svalue"))

        self.retranslateUi(jvsearchdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), jvsearchdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), jvsearchdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(jvsearchdlg)
        jvsearchdlg.setTabOrder(self.searchstring, self.sforward)
        jvsearchdlg.setTabOrder(self.sforward, self.sbackward)
        jvsearchdlg.setTabOrder(self.sbackward, self.wrapround)
        jvsearchdlg.setTabOrder(self.wrapround, self.igncase)
        jvsearchdlg.setTabOrder(self.igncase, self.suser)
        jvsearchdlg.setTabOrder(self.suser, self.stitle)
        jvsearchdlg.setTabOrder(self.stitle, self.sname)
        jvsearchdlg.setTabOrder(self.sname, self.scomment)
        jvsearchdlg.setTabOrder(self.scomment, self.svalue)
        jvsearchdlg.setTabOrder(self.svalue, self.buttonBox)

    def retranslateUi(self, jvsearchdlg):
        jvsearchdlg.setWindowTitle(QtGui.QApplication.translate("jvsearchdlg", "Search for job/variable", None, QtGui.QApplication.UnicodeUTF8))
        self.searchstring.setToolTip(QtGui.QApplication.translate("jvsearchdlg", "This is the string to search for with . for any character", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("jvsearchdlg", "Search for", None, QtGui.QApplication.UnicodeUTF8))
        self.groupBox.setTitle(QtGui.QApplication.translate("jvsearchdlg", "Direction", None, QtGui.QApplication.UnicodeUTF8))
        self.sforward.setToolTip(QtGui.QApplication.translate("jvsearchdlg", "Select to search forwards", None, QtGui.QApplication.UnicodeUTF8))
        self.sforward.setText(QtGui.QApplication.translate("jvsearchdlg", "Forwards", None, QtGui.QApplication.UnicodeUTF8))
        self.sbackward.setToolTip(QtGui.QApplication.translate("jvsearchdlg", "Select to search backwards", None, QtGui.QApplication.UnicodeUTF8))
        self.sbackward.setText(QtGui.QApplication.translate("jvsearchdlg", "Backwards", None, QtGui.QApplication.UnicodeUTF8))
        self.wrapround.setToolTip(QtGui.QApplication.translate("jvsearchdlg", "Select to wrap search round", None, QtGui.QApplication.UnicodeUTF8))
        self.wrapround.setText(QtGui.QApplication.translate("jvsearchdlg", "Wrap round", None, QtGui.QApplication.UnicodeUTF8))
        self.igncase.setToolTip(QtGui.QApplication.translate("jvsearchdlg", "Select this to ignore case differences on letters", None, QtGui.QApplication.UnicodeUTF8))
        self.igncase.setText(QtGui.QApplication.translate("jvsearchdlg", "Ignore Case", None, QtGui.QApplication.UnicodeUTF8))
        self.suser.setToolTip(QtGui.QApplication.translate("jvsearchdlg", "Select to search for user owning job or variable", None, QtGui.QApplication.UnicodeUTF8))
        self.suser.setText(QtGui.QApplication.translate("jvsearchdlg", "User", None, QtGui.QApplication.UnicodeUTF8))
        self.stitle.setToolTip(QtGui.QApplication.translate("jvsearchdlg", "Select to search by job title", None, QtGui.QApplication.UnicodeUTF8))
        self.stitle.setText(QtGui.QApplication.translate("jvsearchdlg", "Title", None, QtGui.QApplication.UnicodeUTF8))
        self.sname.setToolTip(QtGui.QApplication.translate("jvsearchdlg", "Select to search by variable name", None, QtGui.QApplication.UnicodeUTF8))
        self.sname.setText(QtGui.QApplication.translate("jvsearchdlg", "Name", None, QtGui.QApplication.UnicodeUTF8))
        self.scomment.setToolTip(QtGui.QApplication.translate("jvsearchdlg", "Select to search by variable comment", None, QtGui.QApplication.UnicodeUTF8))
        self.scomment.setText(QtGui.QApplication.translate("jvsearchdlg", "Comment", None, QtGui.QApplication.UnicodeUTF8))
        self.svalue.setToolTip(QtGui.QApplication.translate("jvsearchdlg", "Select to search by value of variable", None, QtGui.QApplication.UnicodeUTF8))
        self.svalue.setText(QtGui.QApplication.translate("jvsearchdlg", "Value", None, QtGui.QApplication.UnicodeUTF8))

