#Linux specific building options
PLATFORM=_LINUX
PORTDIR=linux
CC=g++
LINK=g++
LDFLAGS=-Wl,--no-as-needed -ldl
OBJOPT=-o
LIBFLAGS=-shared -lpthread 
LIB_EXT=so
CFLAGS=-O0 -g3 -Wall -c -fmessage-length=0 -fPIC -DLINUX

OMR_CORE="${PYTHON_DIR}/source-python-3.4-omr-linux_x86-64-20140423_197341/bld_linux_x86-64/omr/include_core"
OMR_LIN="${PYTHON_DIR}/source-python-3.4-omr-linux_x86-64-20140423_197341/bld_linux_x86-64/omr/spec/linux_x86-64/include"
PYT_INCS=-I"${PYTHON_DIR}/source-python-3.4-omr-linux_x86-64-20140423_197341/bld_linux_x86-64/cpython/Include" -I"${PYTHON_DIR}/python-3.4-omr-linux_x86-64-20140423_197341/include/python3.4m"

setup: ${OUTPUT}

${OUTPUT}:
	@echo "Creating required build directories under ${OUTPUT}"
	mkdir -p ${OUTPUT}
	mkdir -p ${COMMON_OUT}
	mkdir -p ${PLUGIN_OUT}

clean:
	rm -r ${OUTPUT}