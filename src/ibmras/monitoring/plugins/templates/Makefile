#Relative path to directories
PREFIX=${CURDIR}
PYTHON_DIR=../../../../../../OMR-Python
SRC=../../../../../src/ibmras


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#Output directories
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OUTPUT=${PREFIX}/output
PLUGIN_OUT=${OUTPUT}/plugins
COMMON_OUT=${OUTPUT}/common


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#Objects which make up components
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMON_OBJS = ${COMMON_OUT}/Logging.o ${COMMON_OUT}/LogManager.o ${COMMON_OUT}/File.o ${COMMON_OUT}/Thread.o ${COMMON_OUT}/ThreadData.o ${COMMON_OUT}/Properties.o ${COMMON_OUT}/PropertiesFile.o
PUSHPLUGIN_OBJS=${PLUGIN_OUT}/pushplugin.o
PYPLUGIN_OBJS=${PLUGIN_OUT}/pyplugin.o
PULLPLUGIN_OBJS=${PLUGIN_OUT}/pullplugin.o


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#Compilation
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
INCS=-I"../../../../../src/" -I${SRC} -I${SRC}/ibmras/common
ifndef BUILD
$(error "You must specify a BUILD parameter which contains the name of the platform to build e.g. make BUILD=windows, make BUILD=linux")
endif
default: all
#do not change the position of this include
-include ${BUILD}.mk


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#Components to allow sub-builds
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PUSH=${PLUGIN_OUT}/libpushplugin.so
PYPUSH=${PLUGIN_OUT}/libpyplugin.so
PULL=${PLUGIN_OUT}/libpullplugin.so


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#TARGETS
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

all: setup common pyth push pull
	@echo "All templates built"

pyth: setup common ${PYPUSH}
	@echo "Python template now built"

push: setup common ${PUSH}
	@echo "Push template now built"

pull: setup common ${PULL}
	@echo "Pull template now built"

common: setup ${COMMON_OBJS}
	@echo "Common objects build complete"


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#Libraries
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

${PLUGIN_OUT}/libpushplugin.${LIB_EXT}: ${PUSHPLUGIN_OBJS}
	${LINK} ${LDFLAGS} ${LIBFLAGS} ${OBJOPT}"$@" "$<" ${COMMON_OBJS}
	@echo "Push plugin lib built"
	
${PLUGIN_OUT}/libpyplugin.${LIB_EXT}: ${PYPLUGIN_OBJS}
	${LINK} ${LDFLAGS} ${LIBFLAGS} ${OBJOPT}"$@" "$<" ${COMMON_OBJS}
	@echo "Python plugin lib built"
	
${PLUGIN_OUT}/libpullplugin.${LIB_EXT}: ${PULLPLUGIN_OBJS}
	${LINK} ${LDFLAGS} ${LIBFLAGS} ${OBJOPT}"$@" "$<" ${COMMON_OBJS}
	@echo "Pull plugin lib built"

	
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#Individual object files
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~	

${COMMON_OUT}/File.o: ${SRC}/common/port/${PORTDIR}/File.cpp
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT}"$@" "$<"
	
${COMMON_OUT}/Logging.o: ${SRC}/common/Logging.cpp
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT}"$@" "$<"

${COMMON_OUT}/LogManager.o: ${SRC}/common/LogManager.cpp
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT}"$@" "$<"

${COMMON_OUT}/Thread.o: ${SRC}/common/port/${PORTDIR}/Thread.cpp
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT}"$@" "$<"
	
${COMMON_OUT}/ThreadData.o: ${SRC}/common/port/ThreadData.cpp
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT}"$@" "$<"
	
${COMMON_OUT}/Properties.o: ${SRC}/common/Properties.cpp
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT}"$@" "$<"
	
${COMMON_OUT}/PropertiesFile.o: ${SRC}/common/PropertiesFile.cpp
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT}"$@" "$<"
	
${PLUGIN_OUT}/pushplugin.o: ${SRC}/monitoring/plugins/templates/pushplugin.cpp
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT}"$@" "$<"
	
${PLUGIN_OUT}/pyplugin.o: ${SRC}/monitoring/plugins/pytest/pyplugin.cpp
	${CC} ${INCS} -I${OMR_CORE} -I${OMR_LIN} ${PYT_INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT}"$@" "$<"
	
${PLUGIN_OUT}/pullplugin.o: ${SRC}/monitoring/plugins/templates/pullplugin.cpp
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT}"$@" "$<"

#------------------------------------------------------------
	
.PHONY: clean all common plugins pyth push pull