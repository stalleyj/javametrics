#default installation is relative to this

SRC=./src/ibmras
OUTPUT=./output
#-------------------------------------------------------------------------------------------
#Output directories i.e. where the files are going to be built
#-------------------------------------------------------------------------------------------

COMMON_OUT=${OUTPUT}/common
CONNECTOR_OUT=${OUTPUT}/connectors
AGENT_OUT=${OUTPUT}/agent
PLUGIN_OUT=${OUTPUT}/plugins
TEST_OUT=${OUTPUT}/testharness
INSTALL_DIR=${OUTPUT}/deploy
JAVA_OUT=${OUTPUT}/java
HC_OUT=${JAVA_OUT}

#-------------------------------------------------------------------------------------------
# conditional include of connector directory
#-------------------------------------------------------------------------------------------
COPY_CONNECTOR=cp ${CONNECTOR_OUT}/*.${LIB_EXT} ${INSTALL_DIR}/plugins



#-------------------------------------------------------------------------------------------
#Objects files which make up various components
#-------------------------------------------------------------------------------------------
COMMON_OBJS=${COMMON_OUT}/Logger.o ${COMMON_OUT}/LogManager.o ${COMMON_OUT}/FileUtils.o ${COMMON_OUT}/LibraryUtils.o ${COMMON_OUT}/Thread.o ${COMMON_OUT}/Lock.o ${COMMON_OUT}/Process.o ${COMMON_OUT}/ThreadData.o ${COMMON_OUT}/Properties.o ${COMMON_OUT}/PropertiesFile.o ${COMMON_OUT}/strUtils.o ${COMMON_OUT}/sysUtils.o ${COMMON_OUT}/MemoryManager.o
API_CONNECTOR_OBJS=${CONNECTOR_OUT}/APIConnector.o ${COMMON_OUT}/strUtils.o ${COMMON_OUT}/MemoryManager.o ${COMMON_OUT}/Logger.o ${COMMON_OUT}/LogManager.o ${COMMON_OUT}/Lock.o
AGENT_OBJS=${ASM_OBJS} ${COMMON_OBJS} ${AGENT_OUT}/agent.o ${AGENT_OUT}/ThreadPool.o ${AGENT_OUT}/WorkerThread.o ${AGENT_OUT}/SystemReceiver.o ${AGENT_OUT}/ConnectorManager.o ${AGENT_OUT}/Bucket.o ${AGENT_OUT}/BucketList.o ${AGENT_OUT}/Plugin.o  ${AGENT_OUT}/ConfigurationConnector.o

JAVA_OBJS = ${JAVA_OUT}/Util.o  ${JAVA_OUT}/javametrics.o  ${JAVA_OUT}/JVMTIMemoryManager.o   ${HL_CONNECTOR_OBJS}

ENVPLUGIN_OBJS=${PLUGIN_OUT}/envplugin.o
CPUPLUGIN_OBJS=${PLUGIN_OUT}/cpuplugin.o
MEMPLUGIN_OBJS=${PLUGIN_OUT}/MemoryPlugin.o
TEST_OBJS=${TEST_OUT}/test.o

#-------------------------------------------------------------------------------------------
#Library names
#-------------------------------------------------------------------------------------------
AGENT_LIB=${AGENT_OUT}/agent.${ARC_EXT}
HC_LIB=javametrics.${ARC_EXT}

OSTREAM_LIB=${CONNECTOR_OUT}/ostream.${ARC_EXT}

#-------------------------------------------------------------------------------------------
#Compilation / build configuration parameters
#-------------------------------------------------------------------------------------------
INCS=-Isrc

HC_EXPORT=-DEXPORT
RC_COMPILE=


default: all
#do not change the position of this include
include ${BUILD}.mk


#-------------------------------------------------------------------------------------------
#Components to allow specific sub-builds rather than everything
#-------------------------------------------------------------------------------------------
CONNECTORS=${CONNECTOR_OUT}/${LIB_PREFIX}hcapiplugin.${LIB_EXT}

PLUGINS=${PLUGIN_OUT}/libplugin.${LIB_EXT}
LOG_PLUGIN=${PLUGIN_OUT}/${LIB_PREFIX}logplugin.${LIB_EXT}
COMMON_PLUGINS=${PLUGIN_OUT}/${LIB_PREFIX}envplugin.${LIB_EXT} ${PLUGIN_OUT}/${LIB_PREFIX}cpuplugin.${LIB_EXT} ${PLUGIN_OUT}/${LIB_PREFIX}memoryplugin.${LIB_EXT}
TEST=${TEST_OUT}/test${EXE_EXT}
OBJECTS=${AGENT} ${CONNECTORS} ${PLUGINS} ${TEST}
CORE_AGENT=${JAVA_OUT}/${LIB_PREFIX}agentcore.${LIB_EXT}
JAVA_AGENT=${JAVA_OUT}/${LIB_PREFIX}javametrics.${LIB_EXT}

#-------------------------------------------------------------------------------------------
#Top level targets i.e. those that can be invoked from the command line
#-------------------------------------------------------------------------------------------
all: setup common ${OBJECTS}
	@echo "All components now built"

res:
	${RC_COMPILE}
	@echo "Resource compile complete"

common: setup ${COMMON_OBJS}
	@echo "Common objects build complete"

connectors: setup ${CONNECTORS}
	@echo "Connectors build complete"

agent: setup common ${AGENT}
	@echo "Agent build complete"

plugins: setup common ${PLUGINS}
	@echo "Plugin build complete"

java: setup res common ${JAVA_AGENT} ${CONNECTORS} ${ENVPLUGIN_OBJS} ${CPUPLUGIN_OBJS} ${MEMPLUGIN_OBJS} ${COMMON_PLUGINS}
	@echo "JAVA build complete"

test: setup common ${CONNECTORS} ${AGENT} ${TEST}
	@echo "Test build complete"

#-------------------------------------------------------------------------------------------
#Libraries
#-------------------------------------------------------------------------------------------
${AGENT_OUT}/${LIB_PREFIX}monagent.${LIB_EXT}: ${COMMON_OBJS} ${AGENT_OBJS}
	${LINK} ${LINK_OPT} ${LIBFLAGS} ${LIB_OBJOPT} ${EXELIBS} ${COMMON_OBJS} ${AGENT_OBJS}
	${ARCHIVE} ${AGENT_OBJS}
	@echo "Agent lib built"

${PLUGIN_OUT}/libplugin.${LIB_EXT}: ${TESTPLUGIN_OBJS}
	${LINK} ${LINK_OPT} ${LIBFLAGS} ${LIB_OBJOPT} ${TESTPLUGIN_OBJS} ${COMMON_OBJS} ${EXELIBS}
	@echo "Plugin lib built"

${PLUGIN_OUT}/libosplugin.${LIB_EXT}: ${OSPLUGIN_OBJS}
	${LINK} ${LINK_OPT} ${LIBFLAGS} ${LIB_OBJOPT} ${OSPLUGIN_OBJS} ${COMMON_OBJS} ${EXELIBS}
	@echo "OSPlugin lib built"

${PLUGIN_OUT}/${LIB_PREFIX}cpuplugin.${LIB_EXT}: ${CPUPLUGIN_OBJS}
	${LINK} ${LINK_OPT} ${LIBFLAGS} ${CPUFLAG} ${LIB_OBJOPT} ${CPUPLUGIN_OBJS} ${EXELIBS}
	@echo "CPU lib built"

${PLUGIN_OUT}/${LIB_PREFIX}envplugin.${LIB_EXT}: ${ENVPLUGIN_OBJS}
	${LINK} ${LINK_PLUG} ${LIBFLAGS} ${LIB_OBJOPT} ${ENVPLUGIN_OBJS} ${EXELIBS}
	@echo "Environment lib built"

${PLUGIN_OUT}/${LIB_PREFIX}memoryplugin.${LIB_EXT}: ${MEMPLUGIN_OBJS}
	${LINK} ${LINK_PLUG} ${LIBFLAGS} ${LIB_OBJOPT} ${MEMPLUGIN_OBJS} ${EXELIBS}
	@echo "Memory lib built"


${CONNECTOR_OUT}/${LIB_PREFIX}hcapiplugin.${LIB_EXT}: ${API_CONNECTOR_OBJS} ${COMMON_OBJS}
	${LINK} ${LINK_OPT} ${LIBFLAGS} ${LIB_OBJOPT} ${API_CONNECTOR_OBJS} ${LD_OPT} ${EXELIBS} ${EXEFLAGS}
	@echo "InProcess connector lib built"

${JAVA_OUT}/${LIB_PREFIX}javametrics.${LIB_EXT}: ${AGENT_OBJS} ${JAVA_OBJS}
	${LINK} ${LINK_OPT} ${LIBFLAGS} ${LIB_OBJOPT} ${JAVA_OBJS} ${AGENT_OBJS} ${LD_OPT}  ${EXELIBS} ${EXEFLAGS}
	@echo "JAVA javametrics lib built"

#--------------------------------------------------------------------------------------------
#Test harness
#--------------------------------------------------------------------------------------------
${TEST_OUT}/test${EXE_EXT}: ${TEST_OBJS}
	${LINK} ${LINK_OPT} ${LIBPATH}"${AGENT_OUT}" ${EXEFLAGS} ${LIB_OBJOPT} ${TEST_OBJS} ${EXELIBS} ${MONAGENT} ${LD_OPT} ${COMMON_OBJS}
	@echo "Test harness built"


#---------------------------------------------------------------------------------------------
#Individual object files
#---------------------------------------------------------------------------------------------
${AGENT_OUT}/agent.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/agent/Agent.cpp

${AGENT_OUT}/ThreadPool.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/agent/threads/ThreadPool.cpp

${AGENT_OUT}/WorkerThread.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/agent/threads/WorkerThread.cpp

${AGENT_OUT}/Bucket.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/agent/Bucket.cpp

${AGENT_OUT}/BucketList.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/agent/BucketList.cpp

${AGENT_OUT}/SystemReceiver.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/agent/SystemReceiver.cpp

${AGENT_OUT}/ConnectorManager.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/connector/ConnectorManager.cpp

${AGENT_OUT}/Plugin.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/Plugin.cpp

${AGENT_OUT}/ConfigurationConnector.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/connector/configuration/ConfigurationConnector.cpp

${CONNECTOR_OUT}/APIConnector.o:
	${CC} ${INCS} -I${JAVA_PLAT_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/connector/api/APIConnector.cpp

${PLUGIN_OUT}/plugin.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/test/plugin.cpp

${TEST_OUT}/test.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/testharness/test.cpp

${PLUGIN_OUT}/osplugin.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/os/Plugin.cpp

${PLUGIN_OUT}/os${OS}.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/os/${OS}.cpp

${COMMON_OUT}/FileUtils.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/common/util/FileUtils.cpp

${COMMON_OUT}/LibraryUtils.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/common/util/LibraryUtils.cpp

${JAVA_OUT}/Util.o:
		${CC} ${INCS} -I${JAVA_PLAT_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ./src/Util.cpp

${COMMON_OUT}/Logger.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/common/Logger.cpp

${COMMON_OUT}/LogManager.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/common/LogManager.cpp

${COMMON_OUT}/Thread.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/common/port/${PORTDIR}/Thread.cpp

${COMMON_OUT}/Lock.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/common/port/Lock.cpp

${COMMON_OUT}/Process.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/common/port/${PORTDIR}/Process.cpp

${COMMON_OUT}/ThreadData.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/common/port/ThreadData.cpp

${COMMON_OUT}/Properties.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/common/Properties.cpp

${COMMON_OUT}/PropertiesFile.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/common/PropertiesFile.cpp

${COMMON_OUT}/memUtils.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/common/util/memUtils.cpp

${COMMON_OUT}/MemoryManager.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/common/MemoryManager.cpp

${COMMON_OUT}/strUtils.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/common/util/strUtils.cpp

${COMMON_OUT}/sysUtils.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/common/util/sysUtils.cpp



${PLUGIN_OUT}/envplugin.o: ${SRC}/monitoring/plugins/common/environment/envplugin.cpp
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/common/environment/envplugin.cpp

${PLUGIN_OUT}/cpuplugin.o: ${SRC}/monitoring/plugins/common/cpu/cpuplugin.cpp
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/common/cpu/cpuplugin.cpp

${PLUGIN_OUT}/MemoryPlugin.o: ${SRC}/monitoring/plugins/common/memory/MemoryPlugin.cpp
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/common/memory/MemoryPlugin.cpp

#-------------------------------------------------------------------------------------------
#JAVA vm files which make up various JAVA shim levels
#-------------------------------------------------------------------------------------------

${JAVA_OUT}/javametrics.o:
	${CC} ${INCS} -I${JAVA_PLAT_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ./src/javametrics.cpp

${JAVA_OUT}/JVMTIMemoryManager.o:
	${CC} ${INCS} -I${JAVA_PLAT_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/common/JVMTIMemoryManager.cpp








.PHONY: clean all common agent connectors plugins test install javainstall omrinstall omr


#-------------------------------------------------------------------------------------------
#Various install destinations
#-------------------------------------------------------------------------------------------
setup: ${OUTPUT}

${OUTPUT}:
	@echo "Creating required build directories under ${OUTPUT}"
	mkdir -p ${OUTPUT}
	mkdir -p ${CONNECTOR_OUT}
	mkdir -p ${AGENT_OUT}
	mkdir -p ${COMMON_OUT}
	mkdir -p ${PLUGIN_OUT}
	mkdir -p ${TEST_OUT}
	mkdir -p ${JAVA_OUT}


clean:
	rm -fr ${OUTPUT}

javainstall: java
	@echo "installing to  ${INSTALL_DIR}"
	mkdir -p ${INSTALL_DIR}/plugins
	mkdir -p ${INSTALL_DIR}/libs
	${COPY_CONNECTOR}
	cp src/properties/javametrics.properties ${INSTALL_DIR}
	cp ${JAVA_OUT}/${LIB_PREFIX}javametrics.${LIB_EXT} ${INSTALL_DIR}
	@echo "-----------------------------------------------------------------------------------------------------------------------"
