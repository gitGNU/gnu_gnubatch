dnl   Copyright 2009 Free Software Foundation, Inc.
dnl
dnl  This program is free software: you can redistribute it and/or modify
dnl  it under the terms of the GNU General Public License as published by
dnl  the Free Software Foundation, either version 3 of the License, or
dnl  (at your option) any later version.
dnl
dnl  This program is distributed in the hope that it will be useful,
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl  GNU General Public License for more details.
dnl
dnl   You should have received a copy of the GNU General Public License
dnl  along with this program.  If not, see <http://www.gnu.org/licenses/>.
dnl  Process this file with autoconf to produce a configure script.
dnl
dnl M4 version of resource macros to create segments of resource files.
dnl
divert(-1)

changequote({, })
changecom(//)

// For loops var from to text

define({FORLOOP},
       {pushdef({$1}, {$2})
        _FORLOOP({$1}, {$2}, {$3}, {$4})
        popdef({$1})})

define({_FORLOOP},
       {$4{}ifelse($1,{$3},,{define({$1},incr($1))_FORLOOP({$1}, {$2}, {$3}, {$4})})})

// Utility routines

define({ISUPPER},{ifelse(regexp($1,{^[A-Z]$}),-1,N)})
define({ISLOWER},{ifelse(regexp($1,{^[a-z]$}),-1,N)})
define({TOUPPER},{translit($1,{a-z},{A-Z})})
define({TOUPPERIFSING},{ifelse(regexp($1,{^.$}),-1,{$1},{translit($1,{a-z},{A-Z})})})
define({ISSHIFT},{ifelse(regexp($1,{^[sS]-}),-1,N)})
define({ISCTRL},{ifelse(regexp($1,{^[cC]-}),-1,N)})
define({ISALT},{ifelse(regexp($1,{^[aA]-}),-1,N)})
define({ARGORDEF},{ifelse($1,,$2,$1))})
define({RESTKEY},{TOUPPERIFSING(substr($1,2))})

// Stuff to tab out correctly

define({TAB_SET},
{ifelse(eval($1 % 8),0,,{errprint(Tabs $1 not multiple of 8
)})define({CURRENT_TAB},$1)})

define({TAB_FROM_TO}, {ifelse(eval($1<$2),1,{   TAB_FROM_TO(eval($1+8),$2)})})

// Insert resource

define({RESOURCE_ENTRY},
       {pushdef({ITEM_LENGTH},eval(len($1)+1))
        ifelse(eval(ITEM_LENGTH>=CURRENT_TAB),1,
                {define({CURRENT_TAB},eval(((ITEM_LENGTH+8)/8)*8))errprint(Had to increase tab setting to CURRENT_TAB at __file__ line __line__
)})
        divert(0)$1:TAB_FROM_TO(ITEM_LENGTH,CURRENT_TAB)$2
divert(-1)
popdef({ITEM_LENGTH})})

// Resources for various places

define({TOPLEVEL_RESOURCE}, {RESOURCE_ENTRY(APPNAME.$1, $2)})
define({GLOBAL_RESOURCE}, {RESOURCE_ENTRY(APPNAME*$1, $2)})
define({SECTION_RESOURCE}, {RESOURCE_ENTRY(APPNAME*SECTION.$1, $2)})
define({DLGHDR}, {RESOURCE_ENTRY(APPNAME*SECTION.$1, {$2})})
define({DLGTITLE}, {DLGHDR(dialogTitle, {$1})})
define({DLG_RESOURCE}, {RESOURCE_ENTRY(APPNAME*SECTION*$1, {$2})})
define({DLGITEM_RESOURCE}, {RESOURCE_ENTRY(APPNAME*SECTION*$1.$2, {$3})})
define({MENU_RESOURCE}, {RESOURCE_ENTRY(APPNAME*MENU*$1, $2)})
define({MENUBAR_RESOURCE}, {RESOURCE_ENTRY(APPNAME*MENU.$1.$2, $3)})
define({MENUBUTTON_RESOURCE}, {RESOURCE_ENTRY(APPNAME*MENU*$1.{$2}, {$3})})
define({TOOLBUTTON_RESOURCE}, {RESOURCE_ENTRY(APPNAME*SECTION*$1.$2, {$3})})

// Insert general text

define({XRES_INSERT},{divert(0)$@divert(-1)})

// Insert blank line

define({XRES_SPACE}, {XRES_INSERT({
})})

// Insert comment

define({XRES_COMMENT},{XRES_INSERT({
!! $1})
XRES_SPACE})

// Colours

define({HEX2DIGIT}, {ifelse(eval($1<16),1,{format(0%X,$1)},{format(%X,$1)})})

define({RGB_COLOUR}, {#HEX2DIGIT($1){}HEX2DIGIT($2){}HEX2DIGIT($3)})

define({STANDARD_COLOURS},
       {define({$1_FOREGROUND}, {$2})
        define({$1_BACKGROUND}, {$3})})

// Application - args are application name, title, initial tab setting

define({XRES_APPLICATION},
       {define({APPNAME}, {$1})
        XRES_COMMENT({*******START OF RESOURCES for $1})
        TAB_SET($3)
        TOPLEVEL_RESOURCE(title, {$2})
        XRES_SPACE})

define({XRES_SUBAPP},
       {pushdef({APPNAME}, {$1})
        TOPLEVEL_RESOURCE(title, {$2})
        ifelse({$3},,,{TOPLEVEL_RESOURCE(width, {$3})})
        ifelse({$4},,,{TOPLEVEL_RESOURCE(height, {$4})})
        XRES_SPACE})

define({XRES_ENDSUBAPP}, {popdef({APPNAME})})

define({XRES_END},
       {XRES_SPACE
        XRES_COMMENT(*******END OF RESOURCES for APPNAME)
        XRES_SPACE})

define({XRES_WIDGETOFFSETS},
       {GLOBAL_RESOURCE(leftOffset,ARGORDEF($1,10))
        GLOBAL_RESOURCE(rightOffset, ARGORDEF($2,10))
        GLOBAL_RESOURCE(topOffset, ARGORDEF($3,5))
        GLOBAL_RESOURCE(bottomOffset, ARGORDEF($4,5))})

define({XRES_FOOTER},
       {define({SECTION}, {$1})
        SECTION_RESOURCE(foreground, {FOOTER_FOREGROUND})
        SECTION_RESOURCE(background, {FOOTER_BACKGROUND})
        SECTION_RESOURCE(labelString, {TOUPPER(APPNAME) Copyright Free Software Foundation INC esyscmd({d=`date +%Y`;echo -n $d})})})

define({XRES_PANED},
       {define({SECTION}, {$1})
        SECTION_RESOURCE(foreground, {PANE_FOREGROUND})
        SECTION_RESOURCE(background, {PANE_BACKGROUND})})

define({XRES_TITLE},
       {define({SECTION}, {$1})
        SECTION_RESOURCE(foreground, {TITLE_FOREGROUND})
        SECTION_RESOURCE(background, {TITLE_BACKGROUND})
        SECTION_RESOURCE(alignment, alignment_beginning)
        ifelse({$2},,,{SECTION_RESOURCE(labelString, {$2})})})

define({XRES_CONTEXT},
       {define({SECTION}, {$1})
        SECTION_RESOURCE(foreground, {CONTEXT_FOREGROUND})
        SECTION_RESOURCE(background, {CONTEXT_BACKGROUND})})

define({XRES_LIST},
       {define({SECTION}, {$1})
        SECTION_RESOURCE(foreground, {LIST_FOREGROUND})
        SECTION_RESOURCE(background, {LIST_BACKGROUND})
        ifelse({$2},0,,{SECTION_RESOURCE(visibleItemCount, ARGORDEF($2, 10))})})
define({XRES_VIEWHDR},
       {define({SECTION}, {$1})
        DLG_RESOURCE(foreground, $2{}_FOREGROUND)
        DLG_RESOURCE(background, $2{}_BACKGROUND)})

define({XRES_TOOLBAR},
       {define({SECTION}, {$1})
        ifdef({ICONTOOLS},{define({PANESIZE},ifelse({$3},,{$2},{$3}))},{define({PANESIZE},{$2})})
        DLG_RESOURCE(foreground, {TOOL_FOREGROUND})
        DLG_RESOURCE(background, {TOOL_BACKGROUND})
        SECTION_RESOURCE(paneMinimum, {PANESIZE})
        SECTION_RESOURCE(paneMaximum, {PANESIZE})})

define({XRES_MENU},
       {define({MENU}, {$1})
        pushdef({SECTION}, {$1})
        DLG_RESOURCE(foreground, {MENU_FOREGROUND})
        DLG_RESOURCE(background, {MENU_BACKGROUND})
        DLG_RESOURCE(borderColor,{MENU_BACKGROUND})
        popdef({SECTION})})

define({XRES_TOOLBARITEM},
       {TOOLBUTTON_RESOURCE($1, labelString, {$2})
        ifelse({$3},,,{TOOLBUTTON_RESOURCE($1, toolTipString, {$3})})})

define({XRES_TOOLBARICON},
       {ifdef({ICONTOOLS},{TOOLBUTTON_RESOURCE($1, labelType, pixmap)
                TOOLBUTTON_RESOURCE($1, labelPixmap, @ICONDIR@/{$3})
                TOOLBUTTON_RESOURCE($1, toolTipString, {$4})},
        {XRES_TOOLBARITEM({$1}, {$2}, {$4})})})

define({XRES_POPUPMENU},
       {SECTION_RESOURCE(translations,{#override\
                                <Btn3Down>:     $1()\n})
        XRES_MENU($2)})

define({XRES_TRANSPOPUP},
       {DLGITEM_RESOURCE($1, translations, {#override\
                                <Btn3Down>:     $2()\n})})

define({OVERRIDE_INS},
       {#override \
                                <Key>F5: $1(0) \n\
                                <Key>F6: $1(1) \n\
                                <Key>F7: $1(2) \n\
                                <Key>F8: $1(3) \n\
                                <Key>F9: $1(4) \n\
                                <Key>F10: $1(5) \n\
                                <Key>F11: $1(6) \n\
                                <Key>F12: $1(7) \n\
                                <Key>F2: $1(8) \n\
                                Shift<Key>F2: $1(9) \n\
                                Shift<Key>F3: $1(10) \n\
                                Shift<Key>F4: $1(11) \n\
                                Shift<Key>F5: $1(12) \n\
                                Shift<Key>F6: $1(13) \n\
                                Shift<Key>F7: $1(14) \n\
                                Shift<Key>F8: $1(15) \n\
                                Shift<Key>F9: $1(16) \n\
                                Shift<Key>F10: $1(17) \n\
                                Shift<Key>F11: $1(18) \n\
                                Shift<Key>F12: $1(19)})

define({XRES_STDMACKEYS},
       {DLG_RESOURCE(translations, {OVERRIDE_INS($1)})})

define({XRES_MENUHDR},
       {MENUBAR_RESOURCE($1, labelString, $2)
        MENUBAR_RESOURCE($1, mnemonic, $3)})

define({XRES_ACCEL},
       {ifelse(ISUPPER($1),,
                {MENUBUTTON_RESOURCE(MENUBUTTON, accelerator, {Shift<Key>$1})},
        ISLOWER($1),,
                {MENUBUTTON_RESOURCE(MENUBUTTON, accelerator, {<Key>TOUPPER($1)})},
        ISSHIFT($1),,
                {MENUBUTTON_RESOURCE(MENUBUTTON, accelerator, {Shift<Key>RESTKEY($1)})},
        ISCTRL($1),,
                {MENUBUTTON_RESOURCE(MENUBUTTON, accelerator, {Ctrl<Key>RESTKEY($1)})},
        ISALT($1),,
                {MENUBUTTON_RESOURCE(MENUBUTTON, accelerator, {Meta<Key>RESTKEY($1)})},
        {MENUBUTTON_RESOURCE(MENUBUTTON, accelerator, {<Key>TOUPPERIFSING($1)})})})

define({XRES_ACCELTEXT},
       {ifelse(ISUPPER($1),,
                {MENUBUTTON_RESOURCE(MENUBUTTON, acceleratorText, {Shift+$1})},
        ISLOWER($1),,
                {MENUBUTTON_RESOURCE(MENUBUTTON, acceleratorText, {$1})},
        ISSHIFT($1),,
                {MENUBUTTON_RESOURCE(MENUBUTTON, acceleratorText, {Shift+RESTKEY($1)})},
        ISCTRL($1),,
                {MENUBUTTON_RESOURCE(MENUBUTTON, acceleratorText, {Ctrl+RESTKEY($1)})},
        ISALT($1),,
                {MENUBUTTON_RESOURCE(MENUBUTTON, acceleratorText, {Alt+RESTKEY($1)})},
        {MENUBUTTON_RESOURCE(MENUBUTTON, acceleratorText, {$1})})})

define({XRES_MENUITEM},
       {pushdef({MENUBUTTON}, {$1})
        MENUBUTTON_RESOURCE(MENUBUTTON, labelString, {$2})
        ifelse({$3},,,{MENUBUTTON_RESOURCE(MENUBUTTON, mnemonic, {$3})})
        ifelse({$4},,,
               {XRES_ACCEL($4)
                ifelse($5,,{XRES_ACCELTEXT($4)},{XRES_ACCELTEXT($5)})})
        popdef({MENUBUTTON})})

define({XRES_MACROMENU},
       {pushdef({MENU}, {$1})
        MENUBUTTON_RESOURCE(macro0, labelString, {$2})
        FORLOOP(cnt,1,9,{MENUBUTTON_RESOURCE(macro{}cnt, labelString, {$3 cnt})})
        popdef({MENU})})

// Dialog bits and pieces

define({XRES_STARTDLG},
       {define({SECTION}, {$1})
        DLGTITLE({$2})})

define({XRES_STDDIALOG},
       {XRES_STARTDLG($1, {$2})
        pushdef({UDT},{TOUPPER($1)})
        DLG_RESOURCE(foreground, UDT{_FOREGROUND})
        DLG_RESOURCE(background, UDT{_BACKGROUND})
        popdef({UDT})})

define({XRES_SELDIALOG},
       {XRES_STARTDLG($1, {$2})
        DLG_RESOURCE(foreground, {DIALOG_FOREGROUND})
        DLG_RESOURCE(background, {DIALOG_BACKGROUND})
        DLGHDR(listLabelString, {$3})
        DLGHDR(selectionLabelString, {$4})})

define({XRES_DIALOG},
       {XRES_STARTDLG($1, {$2})
        DLG_RESOURCE(foreground, {DIALOG_FOREGROUND})
        DLG_RESOURCE(background, {DIALOG_BACKGROUND})
        ifelse({$3},,,{DLG_RESOURCE(width, {$3})})})

define({XRES_GENERALLABEL},
       {pushdef({SECTION}, {$1})
        SECTION_RESOURCE(labelString, {$2})
        popdef({SECTION})})

define({XRES_HEIGHT},
       {pushdef({SECTION}, {$1})
        SECTION_RESOURCE(height, {$2})
        popdef({SECTION})})

define({XRES_DLGLABEL},
       {DLGITEM_RESOURCE($1, labelString, {$2})
        ifelse({$3},,,{DLGITEM_RESOURCE($1, showAsDefault, True)})})

define({XRES_DLGLIST}, {DLGITEM_RESOURCE($1, visibleItemCount, {$2})})

define({XRES_DLGCOMBO}, {DLGITEM_RESOURCE($1, columns, {$2})})

define({XRES_DIMS},
       {DLGITEM_RESOURCE($1, width, $2)
        DLGITEM_RESOURCE($1, height, $3)})
