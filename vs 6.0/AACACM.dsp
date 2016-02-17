# Microsoft Developer Studio Project File - Name="AACACM" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=AACACM - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "AACACM.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "AACACM.mak" CFG="AACACM - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "AACACM - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "AACACM - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "AACACM - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AACACM_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\faad2-2.7\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AACACM_EXPORTS" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib winmm.lib /nologo /dll /pdb:none /machine:I386 /out:"..\bin\x86\AACACM.acm"

!ELSEIF  "$(CFG)" == "AACACM - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AACACM_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\faad2-2.7\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AACACM_EXPORTS" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib winmm.lib /nologo /dll /incremental:no /debug /machine:I386 /out:"E:\WINDOWS\system32\AACACM.acm" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "AACACM - Win32 Release"
# Name "AACACM - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\AACACM.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\src\AACACM.def
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\include\neaacdec.h"
# End Source File
# Begin Source File

SOURCE=..\src\Resource.h
# End Source File
# Begin Source File

SOURCE=..\src\WinDDK.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\src\AACACM.rc
# End Source File
# End Group
# Begin Group "Faad2"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;h;hpp;hxx;hm;inl"
# Begin Group "codebook"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\codebook\hcb.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\codebook\hcb_1.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\codebook\hcb_10.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\codebook\hcb_11.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\codebook\hcb_2.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\codebook\hcb_3.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\codebook\hcb_4.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\codebook\hcb_5.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\codebook\hcb_6.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\codebook\hcb_7.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\codebook\hcb_8.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\codebook\hcb_9.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\codebook\hcb_sf.h"
# End Source File
# End Group
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\analysis.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\bits.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\bits.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\cfft.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\cfft.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\cfft_tab.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\common.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\common.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\decoder.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\drc.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\drc.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\drm_dec.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\drm_dec.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\error.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\error.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\filtbank.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\filtbank.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\fixed.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\hcr.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\huffman.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\huffman.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\ic_predict.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\ic_predict.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\iq_table.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\is.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\is.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\kbd_win.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\lt_predict.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\lt_predict.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\mdct.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\mdct.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\mdct_tab.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\mp4.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\mp4.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\ms.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\ms.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\output.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\output.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\pns.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\pns.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\ps_dec.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\ps_dec.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\ps_syntax.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\ps_tables.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\pulse.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\pulse.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\rvlc.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\rvlc.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\sbr_dct.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\sbr_dct.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\sbr_dec.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\sbr_dec.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\sbr_e_nf.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\sbr_e_nf.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\sbr_fbt.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\sbr_fbt.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\sbr_hfadj.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\sbr_hfadj.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\sbr_hfgen.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\sbr_hfgen.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\sbr_huff.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\sbr_huff.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\sbr_noise.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\sbr_qmf.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\sbr_qmf.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\sbr_qmf_c.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\sbr_syntax.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\sbr_syntax.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\sbr_tf_grid.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\sbr_tf_grid.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\sine_win.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\specrec.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\specrec.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\ssr.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\ssr.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\ssr_fb.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\ssr_fb.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\ssr_ipqf.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\ssr_ipqf.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\ssr_win.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\structs.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\syntax.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\syntax.h"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\tns.c"
# End Source File
# Begin Source File

SOURCE="..\faad2-2.7\libfaad\tns.h"
# End Source File
# End Group
# End Target
# End Project
