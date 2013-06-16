# Microsoft Developer Studio Project File - Name="TBoostSTM" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=TBoostSTM - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "TBoostSTM.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TBoostSTM.mak" CFG="TBoostSTM - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TBoostSTM - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "TBoostSTM - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TBoostSTM - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../" /I "../../" /I "../../../" /I "../../../../" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib pthreadVC2.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "TBoostSTM - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../" /I "../../" /I "../../../" /I "../../../../" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib pthreadVC2.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "TBoostSTM - Win32 Release"
# Name "TBoostSTM - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\src\bloom_filter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\contention_manager.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\globalIntArr.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\irrevocableInt.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\isolatedComposedIntLockInTx.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\isolatedComposedIntLockInTx2.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\isolatedInt.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\isolatedIntLockInTx.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\litExample.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\lotExample.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\nestedTxs.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\pointer_test.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\smart.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\stm.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\testatom.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\testBufferedDelete.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\testEmbedded.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\testHashMap.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\testHashMapAndLinkedListsWithLocks.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\testHashMapWithLocks.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\testHT_latm.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\testInt.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\testLinkedList.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\testLinkedListWithLocks.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\testLL_latm.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\testPerson.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\testRBTree.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\transaction.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\transferFun.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\txLinearLock.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\usingLockTx.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\..\boost\stm\detail\auto_lock.hpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\base_transaction.hpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\detail\bit_vector.hpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\detail\bloom_filter.hpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\detail\config.hpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\contention_manager.hpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\detail\datatypes.hpp
# End Source File
# Begin Source File

SOURCE=..\..\test\globalIntArr.h
# End Source File
# Begin Source File

SOURCE=..\..\test\irrevocableInt.h
# End Source File
# Begin Source File

SOURCE=..\..\test\isolatedComposedIntLockInTx.h
# End Source File
# Begin Source File

SOURCE=..\..\test\isolatedComposedIntLockInTx2.h
# End Source File
# Begin Source File

SOURCE=..\..\test\isolatedInt.h
# End Source File
# Begin Source File

SOURCE=..\..\test\isolatedIntLockInTx.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\detail\jenkins_hash.hpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\detail\latm_def_full_impl.hpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\detail\latm_def_tm_impl.hpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\detail\latm_def_tx_impl.hpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\detail\latm_dir_full_impl.hpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\detail\latm_dir_tm_impl.hpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\detail\latm_dir_tx_impl.hpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\detail\latm_general_impl.hpp
# End Source File
# Begin Source File

SOURCE=..\..\test\litExample.h
# End Source File
# Begin Source File

SOURCE=..\..\test\lotExample.h
# End Source File
# Begin Source File

SOURCE=..\..\test\main.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\detail\memory_pool.hpp
# End Source File
# Begin Source File

SOURCE=..\..\test\nestedTxs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\tx\numeric.hpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\tx\pointer.hpp
# End Source File
# Begin Source File

SOURCE=..\..\test\pointer_test.h
# End Source File
# Begin Source File

SOURCE=..\..\test\smart.h
# End Source File
# Begin Source File

SOURCE=..\..\test\testatom.h
# End Source File
# Begin Source File

SOURCE=..\..\test\testBufferedDelete.h
# End Source File
# Begin Source File

SOURCE=..\..\test\testEmbedded.h
# End Source File
# Begin Source File

SOURCE=..\..\test\testHashMap.h
# End Source File
# Begin Source File

SOURCE=..\..\test\testHashMapAndLinkedListsWithLocks.h
# End Source File
# Begin Source File

SOURCE=..\..\test\testHashMapWithLocks.h
# End Source File
# Begin Source File

SOURCE=..\..\test\testHT_latm.h
# End Source File
# Begin Source File

SOURCE=..\..\test\testInt.h
# End Source File
# Begin Source File

SOURCE=..\..\test\testLinkedList.h
# End Source File
# Begin Source File

SOURCE=..\..\test\testLinkedListWithLocks.h
# End Source File
# Begin Source File

SOURCE=..\..\test\testLL_latm.h
# End Source File
# Begin Source File

SOURCE=..\..\test\testPerson.h
# End Source File
# Begin Source File

SOURCE=..\..\test\testRBTree.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\transaction.hpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\detail\transaction_bookkeeping.hpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\detail\transaction_conflict.hpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\detail\transaction_impl.hpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\detail\transaction_impl.hpp~
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\detail\transactions_stack.hpp
# End Source File
# Begin Source File

SOURCE=..\..\test\transferFun.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\detail\tx_ptr.hpp
# End Source File
# Begin Source File

SOURCE=..\..\test\txLinearLock.h
# End Source File
# Begin Source File

SOURCE=..\..\test\usingLockTx.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\detail\vector_map.hpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\boost\stm\detail\vector_set.hpp
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
