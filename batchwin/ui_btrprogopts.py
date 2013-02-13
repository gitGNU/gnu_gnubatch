# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'btrprogopts.ui'
#
# Created: Tue Jan 29 13:25:38 2013
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_btrwprogopts(object):
    def setupUi(self, btrwprogopts):
        btrwprogopts.setObjectName(_fromUtf8("btrwprogopts"))
        btrwprogopts.resize(507, 394)
        self.buttonBox = QtGui.QDialogButtonBox(btrwprogopts)
        self.buttonBox.setGeometry(QtCore.QRect(290, 350, 191, 32))
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.label = QtGui.QLabel(btrwprogopts)
        self.label.setGeometry(QtCore.QRect(30, 40, 101, 17))
        self.label.setObjectName(_fromUtf8("label"))
        self.defserver = QtGui.QComboBox(btrwprogopts)
        self.defserver.setGeometry(QtCore.QRect(140, 40, 191, 27))
        self.defserver.setEditable(True)
        self.defserver.setObjectName(_fromUtf8("defserver"))
        self.useexted = QtGui.QCheckBox(btrwprogopts)
        self.useexted.setGeometry(QtCore.QRect(30, 130, 141, 22))
        self.useexted.setObjectName(_fromUtf8("useexted"))
        self.edname = QtGui.QLineEdit(btrwprogopts)
        self.edname.setEnabled(False)
        self.edname.setGeometry(QtCore.QRect(30, 170, 261, 27))
        self.edname.setObjectName(_fromUtf8("edname"))
        self.interm = QtGui.QCheckBox(btrwprogopts)
        self.interm.setEnabled(False)
        self.interm.setGeometry(QtCore.QRect(40, 210, 121, 22))
        self.interm.setObjectName(_fromUtf8("interm"))
        self.shellarg = QtGui.QLineEdit(btrwprogopts)
        self.shellarg.setEnabled(False)
        self.shellarg.setGeometry(QtCore.QRect(120, 310, 231, 27))
        self.shellarg.setObjectName(_fromUtf8("shellarg"))
        self.label_3 = QtGui.QLabel(btrwprogopts)
        self.label_3.setGeometry(QtCore.QRect(30, 320, 66, 17))
        self.label_3.setObjectName(_fromUtf8("label_3"))
        self.shell = QtGui.QComboBox(btrwprogopts)
        self.shell.setEnabled(False)
        self.shell.setGeometry(QtCore.QRect(120, 260, 231, 27))
        self.shell.setEditable(True)
        self.shell.setObjectName(_fromUtf8("shell"))
        self.shell.addItem(_fromUtf8(""))
        self.shell.addItem(_fromUtf8(""))
        self.shell.addItem(_fromUtf8(""))
        self.label_2 = QtGui.QLabel(btrwprogopts)
        self.label_2.setGeometry(QtCore.QRect(30, 260, 66, 17))
        self.label_2.setObjectName(_fromUtf8("label_2"))
        self.verbose = QtGui.QCheckBox(btrwprogopts)
        self.verbose.setGeometry(QtCore.QRect(30, 90, 111, 22))
        self.verbose.setObjectName(_fromUtf8("verbose"))

        self.retranslateUi(btrwprogopts)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), btrwprogopts.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), btrwprogopts.reject)
        QtCore.QMetaObject.connectSlotsByName(btrwprogopts)
        btrwprogopts.setTabOrder(self.defserver, self.verbose)
        btrwprogopts.setTabOrder(self.verbose, self.useexted)
        btrwprogopts.setTabOrder(self.useexted, self.edname)
        btrwprogopts.setTabOrder(self.edname, self.interm)
        btrwprogopts.setTabOrder(self.interm, self.shell)
        btrwprogopts.setTabOrder(self.shell, self.shellarg)
        btrwprogopts.setTabOrder(self.shellarg, self.buttonBox)

    def retranslateUi(self, btrwprogopts):
        btrwprogopts.setWindowTitle(QtGui.QApplication.translate("btrwprogopts", "Program Options", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("btrwprogopts", "Main Server", None, QtGui.QApplication.UnicodeUTF8))
        self.defserver.setToolTip(QtGui.QApplication.translate("btrwprogopts", "This is the server normally used to submit jobs unless another is selected", None, QtGui.QApplication.UnicodeUTF8))
        self.useexted.setToolTip(QtGui.QApplication.translate("btrwprogopts", "Select this if you want to use an external editor", None, QtGui.QApplication.UnicodeUTF8))
        self.useexted.setText(QtGui.QApplication.translate("btrwprogopts", "External Editor", None, QtGui.QApplication.UnicodeUTF8))
        self.edname.setToolTip(QtGui.QApplication.translate("btrwprogopts", "Give the name of an external editor to use here", None, QtGui.QApplication.UnicodeUTF8))
        self.interm.setToolTip(QtGui.QApplication.translate("btrwprogopts", "Run external editor via shell", None, QtGui.QApplication.UnicodeUTF8))
        self.interm.setText(QtGui.QApplication.translate("btrwprogopts", "Run in shell", None, QtGui.QApplication.UnicodeUTF8))
        self.shellarg.setToolTip(QtGui.QApplication.translate("btrwprogopts", "<html><head/><body><p>This is the argument to be passed to the terminal program to make it take the rest of the command line as arguments to passed to the editor.</p><p>For <span style=\" font-weight:600;\">xterm</span> and <span style=\" font-weight:600;\">konsole</span> the argument <span style=\" font-weight:600;\">-e</span> is used.</p><p>For <span style=\" font-weight:600;\">gnome-terminal</span> the argumens <span style=\" font-weight:600;\">--disable-factory</span><span style=\" font-weight:600;\">-x</span> is used.</p><p>For other terminal programs you may need to consult the manual<br/></p></body></html>", None, QtGui.QApplication.UnicodeUTF8))
        self.shellarg.setText(QtGui.QApplication.translate("btrwprogopts", "e", None, QtGui.QApplication.UnicodeUTF8))
        self.label_3.setText(QtGui.QApplication.translate("btrwprogopts", "Argument", None, QtGui.QApplication.UnicodeUTF8))
        self.shell.setToolTip(QtGui.QApplication.translate("btrwprogopts", "<html><head/><body><p>For external editors which have to be run in a shell, this is the name of the terminal program to use.</p><p>Alternatives of <span style=\" font-style:italic;\">xterm</span>, <span style=\" font-style:italic;\">konsole</span> and <span style=\" font-style:italic;\">gnome-terminal</span> are known about, but you can put another one in if you wish.</p><p>You can add additional options like <span style=\" font-weight:600;\">-bg blue</span> or similar here</p><p>In most cases you will have to supply an argument to the command to make it interpret the rest of the command line as arguments to the editor, which is done in the following argument.</p></body></html>", None, QtGui.QApplication.UnicodeUTF8))
        self.shell.setItemText(0, QtGui.QApplication.translate("btrwprogopts", "xterm", None, QtGui.QApplication.UnicodeUTF8))
        self.shell.setItemText(1, QtGui.QApplication.translate("btrwprogopts", "gnome-terminal", None, QtGui.QApplication.UnicodeUTF8))
        self.shell.setItemText(2, QtGui.QApplication.translate("btrwprogopts", "konsole", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("btrwprogopts", "Terminal", None, QtGui.QApplication.UnicodeUTF8))
        self.verbose.setToolTip(QtGui.QApplication.translate("btrwprogopts", "Display results of submission even if successful", None, QtGui.QApplication.UnicodeUTF8))
        self.verbose.setText(QtGui.QApplication.translate("btrwprogopts", "Verbose", None, QtGui.QApplication.UnicodeUTF8))

