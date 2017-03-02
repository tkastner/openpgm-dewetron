@echo off

set PGM_LIB_PATH=build
set OUTPUT_LIB_PATH=%2
set OUTPUT_INC_PATH=%3

if "%1"=="x64" (
  echo 64bit build selected
  set PGM_LIB_PATH="build64"
  set OUTPUT_LIB_PATH="%OUTPUT_LIB_PATH%64"
) else (
  echo 32bit build selected
)

REM prepare destination directories

set OUTPUT_LIB_PATH_DEBUG="%OUTPUT_LIB_PATH%\static\Debug"
set OUTPUT_LIB_PATH_RELEASE="%OUTPUT_LIB_PATH%\static\Release"

mkdir "%OUTPUT_LIB_PATH_DEBUG%"
mkdir "%OUTPUT_LIB_PATH_RELEASE%"

REM static lib
copy "%PGM_LIB_PATH%\Debug\libpgm*.lib" "%OUTPUT_LIB_PATH_DEBUG%\"
copy "%PGM_LIB_PATH%\Release\libpgm*.lib" "%OUTPUT_LIB_PATH_RELEASE%\"


mkdir "%OUTPUT_INC_PATH%"\pgm
REM copy "include\pgm\*.h" "%OUTPUT_INC_PATH%\pgm\"

copy include\pgm\atomic.h "%OUTPUT_INC_PATH%\pgm\"
copy include\pgm\engine.h "%OUTPUT_INC_PATH%\pgm\"
copy include\pgm\error.h "%OUTPUT_INC_PATH%\pgm\"
copy include\pgm\gsi.h "%OUTPUT_INC_PATH%\pgm\"
copy include\pgm\if.h "%OUTPUT_INC_PATH%\pgm\"
copy include\pgm\in.h "%OUTPUT_INC_PATH%\pgm\"
copy include\pgm\list.h "%OUTPUT_INC_PATH%\pgm\"
copy include\pgm\macros.h "%OUTPUT_INC_PATH%\pgm\"
copy include\pgm\mem.h "%OUTPUT_INC_PATH%\pgm\"
copy include\pgm\messages.h "%OUTPUT_INC_PATH%\pgm\"
copy include\pgm\msgv.h "%OUTPUT_INC_PATH%\pgm\"
copy include\pgm\packet.h "%OUTPUT_INC_PATH%\pgm\"
copy include\pgm\pgm.h "%OUTPUT_INC_PATH%\pgm\"
copy include\pgm\skbuff.h "%OUTPUT_INC_PATH%\pgm\"
copy include\pgm\socket.h "%OUTPUT_INC_PATH%\pgm\"
copy include\pgm\time.h "%OUTPUT_INC_PATH%\pgm\"
copy include\pgm\tsi.h "%OUTPUT_INC_PATH%\pgm\"
copy include\pgm\types.h "%OUTPUT_INC_PATH%\pgm\"
copy include\pgm\version.h "%OUTPUT_INC_PATH%\pgm\"
copy include\pgm\winint.h "%OUTPUT_INC_PATH%\pgm\"
copy include\pgm\wininttypes.h "%OUTPUT_INC_PATH%\pgm\"
copy include\pgm\zinttypes.h "%OUTPUT_INC_PATH%\pgm\"
