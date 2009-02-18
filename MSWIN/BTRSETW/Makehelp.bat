@echo off
PATH=C:\WINDOWS\COMMAND;C:\PROGRA~1\DEVSTUDIO\VC\BIN
REM -- First make map file from App Studio generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by BTRSETW.HPJ. >hlp\btrsetw.hm
echo. >>hlp\btrsetw.hm
echo // Commands (ID_* and IDM_*) >>hlp\btrsetw.hm
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>hlp\btrsetw.hm
echo. >>hlp\btrsetw.hm
echo // Prompts (IDP_*) >>hlp\btrsetw.hm
makehm IDP_,HIDP_,0x30000 resource.h >>hlp\btrsetw.hm
echo. >>hlp\btrsetw.hm
echo // Resources (IDR_*) >>hlp\btrsetw.hm
makehm IDR_,HIDR_,0x20000 resource.h >>hlp\btrsetw.hm
echo. >>hlp\btrsetw.hm
echo // Dialogs (IDD_*) >>hlp\btrsetw.hm
makehm IDD_,HIDD_,0x20000 resource.h >>hlp\btrsetw.hm
echo. >>hlp\btrsetw.hm
echo // Frame Controls (IDW_*) >>hlp\btrsetw.hm
makehm IDW_,HIDW_,0x50000 resource.h >>hlp\btrsetw.hm
echo.
