#include makefile for AIX
PLATFORM=_AIX
PORTDIR=aix
CC=g++
LINK=g++
GCC=gcc
LINK_OPT=
LD_OPT=-Wl,-bexpall,-brtllib,-G,-bernotok,-brtl -maix64 -shared -fPIC -pthread
JAVA_PLAT_INCLUDE=${JAVA_SDK_INCLUDE}
OBJOPT=-o"$@"
ARCHIVE=ar -r 
ARCHIVE_MQTT=ar -r ${MQTT_LIB} 
ARC_EXT=a
CFLAGS=-O3 -Wall -pthread -c -fmessage-length=0 -std=c++0x -maix64 -D__BIG_ENDIAN -D_AIX -DAIX -DAIXPPC -D_64BIT -fPIC -DREVERSED -D_AIX -mcpu=powerpc64
LIB_EXT=so
EXE_EXT=
LIBFLAGS=-Wl,-bexpall,-brtllib,-G,-bernotok,-brtl -maix64 -shared -fPIC -pthread -ldl 
CPUFLAG=-lperfstat
LIB_OBJOPT=-o"$@"
LIBPATH=-L
EXEFLAGS=
LIB_PREFIX=lib
#ifdef NODE_SDK
NODE_GYP=PATH=${NODE_SDK}/bin:$$PATH ${NODE_SDK}/lib/node_modules/npm/bin/node-gyp-bin/node-gyp ${OPT_PYTHON}
#endif