@echo off
REM -- First make map file from App Studio generated resource.h
PATH=C:\WINDOWS\COMMAND;C:\PROGRA~1\DEVSTUDIO\VC\BIN
echo // MAKEHELP.BAT generated Help Map file.  Used by BTQW.HPJ. >hlp\BTQW.hm
echo. >>hlp\BTQW.hm
echo // Commands (ID_* and IDM_*) >>hlp\BTQW.hm
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>hlp\BTQW.hm
echo. >>hlp\BTQW.hm
echo // Prompts (IDP_*) >>hlp\BTQW.hm
makehm IDP_,HIDP_,0x30000 resource.h >>hlp\BTQW.hm
echo. >>hlp\BTQW.hm
echo // Resources (IDR_*) >>hlp\BTQW.hm
makehm IDR_,HIDR_,0x20000 resource.h >>hlp\BTQW.hm
echo. >>hlp\BTQW.hm
echo // Dialogs (IDD_*) >>hlp\BTQW.hm
makehm IDD_,HIDD_,0x20000 resource.h >>hlp\BTQW.hm
echo. >>hlp\BTQW.hm
echo // Frame Controls (IDW_*) >>hlp\BTQW.hm
makehm IDW_,HIDW_,0x50000 resource.h >>hlp\BTQW.hm
echo.
