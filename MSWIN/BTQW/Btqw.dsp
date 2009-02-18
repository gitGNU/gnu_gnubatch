# Microsoft Developer Studio Project File - Name="Btqw" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Btqw - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Btqw.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Btqw.mak" CFG="Btqw - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Btqw - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Btqw - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Btqw - Win32 Release"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G3 /MT /W3 /GX /O2 /Ob2 /I "..\include" /I "..\WINLIB" /I "." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "BTQW" /FR /Yu"STDAFX.H" /FD /c
# ADD CPP /nologo /G3 /MT /W3 /GX /O2 /Ob2 /I "..\BTQW" /I "..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "BTQW" /FR /Yu"STDAFX.H" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winsock.lib oldnames.lib /nologo /stack:0x55f0 /subsystem:windows /map /machine:IX86
# ADD LINK32 wsock32.lib oldnames.lib /nologo /stack:0x55f0 /subsystem:windows /map /machine:IX86

!ELSEIF  "$(CFG)" == "Btqw - Win32 Debug"

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
# ADD BASE CPP /nologo /G3 /MTd /W3 /Gm /GX /Zi /Od /I "..\include" /I "..\WINLIB" /I "." /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "BTQW" /FR /Yu"STDAFX.H" /FD /c
# ADD CPP /nologo /G3 /MTd /W3 /Gm /GX /ZI /Od /I "..\WINLIB" /I "." /I "..\include" /I "..\BTQW" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "BTQW" /FR /Yu"STDAFX.H" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winsock.lib oldnames.lib /nologo /stack:0x55f0 /subsystem:windows /debug /machine:IX86 /pdbtype:sept
# ADD LINK32 wsock32.lib oldnames.lib /nologo /subsystem:windows /debug /machine:IX86 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Btqw - Win32 Release"
# Name "Btqw - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat"
# Begin Source File

SOURCE=..\Winlib\Advtime.cpp
# End Source File
# Begin Source File

SOURCE=.\Argdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Assvar.cpp
# End Source File
# Begin Source File

SOURCE=.\Btmode.cpp
# End Source File
# Begin Source File

SOURCE=.\Btqw.cpp
# End Source File
# Begin Source File

SOURCE=.\Btqw.def
# End Source File
# Begin Source File

SOURCE=.\Btqw.rc
# End Source File
# Begin Source File

SOURCE=.\Constval.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\conthelp.cpp
# End Source File
# Begin Source File

SOURCE=.\Cpyoptdl.cpp
# End Source File
# Begin Source File

SOURCE=.\Defass.cpp
# End Source File
# Begin Source File

SOURCE=.\Defcond.cpp
# End Source File
# Begin Source File

SOURCE=.\Deftimop.cpp
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

SOURCE=.\Editfmt.cpp
# End Source File
# Begin Source File

SOURCE=.\Editredi.cpp
# End Source File
# Begin Source File

SOURCE=.\Editsep.cpp
# End Source File
# Begin Source File

SOURCE=.\Envlist.cpp
# End Source File
# Begin Source File

SOURCE=.\Fmtdef.cpp
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

SOURCE=.\Jdatadoc.cpp
# End Source File
# Begin Source File

SOURCE=.\Jdatavie.cpp
# End Source File
# Begin Source File

SOURCE=.\Jdsearch.cpp
# End Source File
# Begin Source File

SOURCE=.\Jobcolours.cpp
# End Source File
# Begin Source File

SOURCE=.\Jobdoc.cpp
# End Source File
# Begin Source File

SOURCE=.\Joblist.cpp
# End Source File
# Begin Source File

SOURCE=.\Jobview.cpp
# End Source File
# Begin Source File

SOURCE=.\Jperm.cpp
# End Source File
# Begin Source File

SOURCE=.\Jsearch.cpp
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

SOURCE=.\Netinit.cpp
# End Source File
# Begin Source File

SOURCE=.\Netmsg.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\netrouts.cpp
# End Source File
# Begin Source File

SOURCE=.\Netstat.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\Optcopy.cpp
# End Source File
# Begin Source File

SOURCE=.\Othersig.cpp
# End Source File
# Begin Source File

SOURCE=.\Packem.cpp
# End Source File
# Begin Source File

SOURCE=.\Poptsdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Procpar.cpp
# End Source File
# Begin Source File

SOURCE=.\Redirlis.cpp
# End Source File
# Begin Source File

SOURCE=.\Restrict.cpp
# End Source File
# Begin Source File

SOURCE=.\Rowview.cpp
# End Source File
# Begin Source File

SOURCE=.\Smstr.cpp
# End Source File
# Begin Source File

SOURCE=.\Stdafx.cpp
# ADD BASE CPP /Yc"STDAFX.H"
# ADD CPP /Yc"STDAFX.H"
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

SOURCE=.\Ulist.cpp
# End Source File
# Begin Source File

SOURCE=.\Unqueue.cpp
# End Source File
# Begin Source File

SOURCE=.\Uperms.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\userops.cpp
# End Source File
# Begin Source File

SOURCE=.\Varcomm.cpp
# End Source File
# Begin Source File

SOURCE=.\Vardoc.cpp
# End Source File
# Begin Source File

SOURCE=.\Varlist.cpp
# End Source File
# Begin Source File

SOURCE=.\Varperm.cpp
# End Source File
# Begin Source File

SOURCE=.\Varview.cpp
# End Source File
# Begin Source File

SOURCE=.\Vsearch.cpp
# End Source File
# Begin Source File

SOURCE=.\Winopts.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\Argdlg.h
# End Source File
# Begin Source File

SOURCE=.\Assvar.h
# End Source File
# Begin Source File

SOURCE=..\Include\Btenvir.h
# End Source File
# Begin Source File

SOURCE=.\Btqw.h
# End Source File
# Begin Source File

SOURCE=.\Btqw.hpp
# End Source File
# Begin Source File

SOURCE=.\Constval.h
# End Source File
# Begin Source File

SOURCE=.\Cpyoptdl.h
# End Source File
# Begin Source File

SOURCE=.\Defass.h
# End Source File
# Begin Source File

SOURCE=.\Defcond.h
# End Source File
# Begin Source File

SOURCE=.\Deftimop.h
# End Source File
# Begin Source File

SOURCE=.\Editarg.h
# End Source File
# Begin Source File

SOURCE=.\Editass.h
# End Source File
# Begin Source File

SOURCE=.\Editcond.h
# End Source File
# Begin Source File

SOURCE=.\Editenv.h
# End Source File
# Begin Source File

SOURCE=.\Editfmt.h
# End Source File
# Begin Source File

SOURCE=.\Editredi.h
# End Source File
# Begin Source File

SOURCE=.\Editsep.h
# End Source File
# Begin Source File

SOURCE=.\Envlist.h
# End Source File
# Begin Source File

SOURCE=.\Fmtdef.h
# End Source File
# Begin Source File

SOURCE=.\formatcode.h
# End Source File
# Begin Source File

SOURCE=.\Jasslist.h
# End Source File
# Begin Source File

SOURCE=.\Jcondlis.h
# End Source File
# Begin Source File

SOURCE=.\Jdatadoc.h
# End Source File
# Begin Source File

SOURCE=.\Jdatavie.h
# End Source File
# Begin Source File

SOURCE=.\Jdsearch.h
# End Source File
# Begin Source File

SOURCE=.\Jobcolours.h
# End Source File
# Begin Source File

SOURCE=.\Jobdoc.h
# End Source File
# Begin Source File

SOURCE=.\Joblist.h
# End Source File
# Begin Source File

SOURCE=.\Jobview.h
# End Source File
# Begin Source File

SOURCE=.\Jperm.h
# End Source File
# Begin Source File

SOURCE=.\Jsearch.h
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

SOURCE=..\Include\Netmsg.h
# End Source File
# Begin Source File

SOURCE=.\Netstat.h
# End Source File
# Begin Source File

SOURCE=.\Netwmsg.h
# End Source File
# Begin Source File

SOURCE=.\Othersig.h
# End Source File
# Begin Source File

SOURCE=.\Poptsdlg.h
# End Source File
# Begin Source File

SOURCE=.\Procpar.h
# End Source File
# Begin Source File

SOURCE=.\Redirlis.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\Restrict.h
# End Source File
# Begin Source File

SOURCE=.\Rowview.h
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

SOURCE=.\Ulist.h
# End Source File
# Begin Source File

SOURCE=.\Uperms.h
# End Source File
# Begin Source File

SOURCE=.\Varcomm.h
# End Source File
# Begin Source File

SOURCE=.\Vardoc.h
# End Source File
# Begin Source File

SOURCE=.\Varlist.h
# End Source File
# Begin Source File

SOURCE=.\Varperm.h
# End Source File
# Begin Source File

SOURCE=.\Varview.h
# End Source File
# Begin Source File

SOURCE=.\Vsearch.h
# End Source File
# Begin Source File

SOURCE=.\Winopts.h
# End Source File
# Begin Source File

SOURCE=..\Include\xbnetq.h
# End Source File
# Begin Source File

SOURCE=..\Include\xbwnetwk.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\RES\BTQW.ICO
# End Source File
# Begin Source File

SOURCE=.\res\btqw.rc2
# End Source File
# Begin Source File

SOURCE=.\RES\COPYCURS.CUR
# End Source File
# Begin Source File

SOURCE=.\RES\IDR_MAIN.BMP
# End Source File
# Begin Source File

SOURCE=.\Res\idr_view.ico
# End Source File
# Begin Source File

SOURCE=.\RES\JOBS.ICO
# End Source File
# Begin Source File

SOURCE=.\RES\MOVECURS.CUR
# End Source File
# Begin Source File

SOURCE=.\RES\VARS.ICO
# End Source File
# Begin Source File

SOURCE=.\RES\VIEWJOB.ICO
# End Source File
# End Group
# End Target
# End Project
