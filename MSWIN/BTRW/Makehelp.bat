@echo off
REM -- First make map file from App Studio generated resource.h
PATH=C:\WINDOWS\COMMAND;C:\PROGRA~1\DEVSTUDIO\VC\BIN
echo // MAKEHELP.BAT generated Help Map file.  Used by BTRW.HPJ. >hlp\btrw.hm
echo. >>hlp\btrw.hm
echo // Commands (ID_* and IDM_*) >>hlp\btrw.hm
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>hlp\btrw.hm
echo. >>hlp\btrw.hm
echo // Prompts (IDP_*) >>hlp\btrw.hm
makehm IDP_,HIDP_,0x30000 resource.h >>hlp\btrw.hm
echo. >>hlp\btrw.hm
echo // Resources (IDR_*) >>hlp\btrw.hm
makehm IDR_,HIDR_,0x20000 resource.h >>hlp\btrw.hm
echo. >>hlp\btrw.hm
echo // Dialogs (IDD_*) >>hlp\btrw.hm
makehm IDD_,HIDD_,0x20000 resource.h >>hlp\btrw.hm
echo. >>hlp\btrw.hm
echo // Frame Controls (IDW_*) >>hlp\btrw.hm
makehm IDW_,HIDW_,0x50000 resource.h >>hlp\btrw.hm
echo.
