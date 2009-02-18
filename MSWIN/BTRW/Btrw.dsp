# Microsoft Developer Studio Project File - Name="Btrw" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Btrw - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Btrw.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Btrw.mak" CFG="Btrw - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Btrw - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Btrw - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Btrw - Win32 Release"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G3 /MT /W3 /GX /O2 /Ob2 /I "..\include" /I "..\WINLIB" /I "..\BTRW" /I "..\BTQW" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "BTRW" /FR /Yu"STDAFX.H" /FD /c
# ADD CPP /nologo /G3 /MD /W3 /GX /O2 /Ob2 /I "..\include" /I "..\BTRW" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "BTRW" /D "_AFXDLL" /FR /Yu"STDAFX.H" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 oldnames.lib winsock.lib /nologo /stack:0x5000 /subsystem:windows /machine:IX86
# ADD LINK32 wsock32.lib /nologo /stack:0x5000 /subsystem:windows /machine:IX86

!ELSEIF  "$(CFG)" == "Btrw - Win32 Debug"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\include" /I "..\WINLIB" /I "..\BTRW" /I "..\BTQW" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "BTRW" /FR /Yu"STDAFX.H" /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\include" /I "..\BTRW" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "BTRW" /FR /Yu"STDAFX.H" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 oldnames.lib winsock.lib /nologo /stack:0x5000 /subsystem:windows /debug /machine:IX86 /pdbtype:sept
# ADD LINK32 wsock32.lib /nologo /stack:0x5000 /subsystem:windows /debug /machine:IX86 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Btrw - Win32 Release"
# Name "Btrw - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat"
# Begin Source File

SOURCE=..\Winlib\Advtime.cpp
# End Source File
# Begin Source File

SOURCE=.\Argdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Btjobcd.cpp
# End Source File
# Begin Source File

SOURCE=.\Btrw.cpp
# End Source File
# Begin Source File

SOURCE=.\Btrw.def
# End Source File
# Begin Source File

SOURCE=.\Btrw.rc
# End Source File
# Begin Source File

SOURCE=.\Btrwdoc.cpp
# End Source File
# Begin Source File

SOURCE=.\Btrwview.cpp
# End Source File
# Begin Source File

SOURCE=.\Chkminmd.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\conthelp.cpp
# End Source File
# Begin Source File

SOURCE=.\Defass.cpp
# End Source File
# Begin Source File

SOURCE=.\Defcond.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\Dumpjob.cpp
# End Source File
# Begin Source File

SOURCE=.\Editarg.cpp
# End Source File
# Begin Source File

SOURCE=.\Editass.cpp
# End Source File
# Begin Source File

SOURCE=.\Editcond.cpp
# End Source File
# Begin Source File

SOURCE=.\Editenv.cpp
# End Source File
# Begin Source File

SOURCE=.\editredi.cpp
# End Source File
# Begin Source File

SOURCE=.\Envlist.cpp
# End Source File
# Begin Source File

SOURCE=.\Envtable.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\Getregdata.cpp
# End Source File
# Begin Source File

SOURCE=.\Jasslist.cpp
# End Source File
# Begin Source File

SOURCE=.\Jcondlis.cpp
# End Source File
# Begin Source File

SOURCE=.\Jperm.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\LoadHost.cpp
# End Source File
# Begin Source File

SOURCE=.\LoginHost.cpp
# End Source File
# Begin Source File

SOURCE=.\Maildlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Mainfrm.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\netrouts.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\Optcopy.cpp
# End Source File
# Begin Source File

SOURCE=.\Procpar.cpp
# End Source File
# Begin Source File

SOURCE=.\Progopts.cpp
# End Source File
# Begin Source File

SOURCE=.\Redirlis.cpp
# End Source File
# Begin Source File

SOURCE=.\Speakudp.cpp
# End Source File
# Begin Source File

SOURCE=.\Stdafx.cpp
# ADD BASE CPP /Yc"STDAFX.H"
# ADD CPP /Yc"STDAFX.H"
# End Source File
# Begin Source File

SOURCE=.\Submitj.cpp
# End Source File
# Begin Source File

SOURCE=.\Timedlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Timelim.cpp
# End Source File
# Begin Source File

SOURCE=.\Titledlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Uperms.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\userops.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\Argdlg.h
# End Source File
# Begin Source File

SOURCE=.\Btrw.h
# End Source File
# Begin Source File

SOURCE=.\Btrw.hpp
# End Source File
# Begin Source File

SOURCE=.\Btrwdoc.h
# End Source File
# Begin Source File

SOURCE=.\Btrwview.h
# End Source File
# Begin Source File

SOURCE=.\Defass.h
# End Source File
# Begin Source File

SOURCE=.\defcond.h
# End Source File
# Begin Source File

SOURCE=.\Editarg.h
# End Source File
# Begin Source File

SOURCE=.\editass.h
# End Source File
# Begin Source File

SOURCE=.\editcond.h
# End Source File
# Begin Source File

SOURCE=.\Editenv.h
# End Source File
# Begin Source File

SOURCE=.\editredi.h
# End Source File
# Begin Source File

SOURCE=.\Envlist.h
# End Source File
# Begin Source File

SOURCE=.\Jasslist.h
# End Source File
# Begin Source File

SOURCE=.\Jcondlis.h
# End Source File
# Begin Source File

SOURCE=.\Jperm.h
# End Source File
# Begin Source File

SOURCE=.\LoginHost.h
# End Source File
# Begin Source File

SOURCE=.\Maildlg.h
# End Source File
# Begin Source File

SOURCE=.\Mainfrm.h
# End Source File
# Begin Source File

SOURCE=.\Procpar.h
# End Source File
# Begin Source File

SOURCE=.\Progopts.h
# End Source File
# Begin Source File

SOURCE=.\Redirlis.h
# End Source File
# Begin Source File

SOURCE=.\Stdafx.h
# End Source File
# Begin Source File

SOURCE=.\Timedlg.h
# End Source File
# Begin Source File

SOURCE=.\Timelim.h
# End Source File
# Begin Source File

SOURCE=.\Titledlg.h
# End Source File
# Begin Source File

SOURCE=..\Include\Ulist.h
# End Source File
# Begin Source File

SOURCE=.\Uperms.h
# End Source File
# Begin Source File

SOURCE=..\Include\xbwnetwk.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\RES\BTRW.ICO
# End Source File
# Begin Source File

SOURCE=.\res\btrw.rc2
# End Source File
# Begin Source File

SOURCE=.\RES\TOOLBAR.BMP
# End Source File
# End Group
# End Target
# End Project
