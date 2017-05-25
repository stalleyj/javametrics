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
OMR_OUT=${OUTPUT}/omr
NODE_OUT=${OUTPUT}/node
PYTHON_OUT=${OUTPUT}/python
RUBY_OUT=${OUTPUT}/ruby
#CORE_OUT=${OUTPUT}/core
HC_OUT=${JAVA_OUT}

#-------------------------------------------------------------------------------------------
# conditional include of connector directory
#-------------------------------------------------------------------------------------------
COPY_CONNECTOR=cp ${CONNECTOR_OUT}/*.${LIB_EXT} ${INSTALL_DIR}/plugins



#-------------------------------------------------------------------------------------------
#Objects files which make up various components
#-------------------------------------------------------------------------------------------
COMMON_OBJS=${COMMON_OUT}/Logger.o ${COMMON_OUT}/LogManager.o ${COMMON_OUT}/FileUtils.o ${COMMON_OUT}/LibraryUtils.o ${COMMON_OUT}/Thread.o ${COMMON_OUT}/Lock.o ${COMMON_OUT}/Process.o ${COMMON_OUT}/ThreadData.o ${COMMON_OUT}/Properties.o ${COMMON_OUT}/PropertiesFile.o ${COMMON_OUT}/strUtils.o ${COMMON_OUT}/sysUtils.o ${COMMON_OUT}/MemoryManager.o
HL_CONNECTOR_OBJS=${CONNECTOR_OUT}/HLConnector.o ${CONNECTOR_OUT}/HLConnectorPlugin.o
API_CONNECTOR_OBJS=${CONNECTOR_OUT}/APIConnector.o ${COMMON_OUT}/strUtils.o ${COMMON_OUT}/MemoryManager.o ${COMMON_OUT}/Logger.o ${COMMON_OUT}/LogManager.o ${COMMON_OUT}/Lock.o
AGENT_OBJS=${ASM_OBJS} ${COMMON_OBJS} ${AGENT_OUT}/agent.o ${AGENT_OUT}/ThreadPool.o ${AGENT_OUT}/WorkerThread.o ${AGENT_OUT}/SystemReceiver.o ${AGENT_OUT}/ConnectorManager.o ${AGENT_OUT}/Bucket.o ${AGENT_OUT}/BucketList.o ${AGENT_OUT}/Plugin.o  ${AGENT_OUT}/ConfigurationConnector.o

OSTREAM_CONNECTOR_OBJS = ${CONNECTOR_OUT}/OStreamConnector.o
TESTPLUGIN_OBJS=${PLUGIN_OUT}/plugin.o
OSPLUGIN_OBJS=${PLUGIN_OUT}/osplugin.o ${PLUGIN_OUT}/os${OS}.o

JAVA_OBJS = ${JAVA_OUT}/DumpHandler.o ${JAVA_OUT}/Util.o ${JAVA_OUT}/ClassHistogramProvider.o ${JAVA_OUT}/TraceDataProvider.o ${JAVA_OUT}/TraceReceiver.o  ${JAVA_OUT}/JMXConnector.o ${JAVA_OUT}/JMXConnectorPlugin.o ${JAVA_OUT}/healthcenter.o  ${JAVA_OUT}/JVMTIMemoryManager.o ${JAVA_OUT}/MethodLookupProvider.o ${JAVA_OUT}/EnvironmentPlugin.o ${JAVA_OUT}/ThreadsPlugin.o ${JAVA_OUT}/MemoryPlugin.o ${JAVA_OUT}/MemCountersPlugin.o ${JAVA_OUT}/CpuPlugin.o ${JAVA_OUT}/AppPlugin.o ${JAVA_OUT}/LockingPlugin.o ${HL_CONNECTOR_OBJS}

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

NODE_SDK_INCLUDE=${NODE_SDK}/include/node


#-------------------------------------------------------------------------------------------
#Components to allow specific sub-builds rather than everything
#-------------------------------------------------------------------------------------------
CONNECTORS=${CONNECTOR_OUT}/${LIB_PREFIX}hcapiplugin.${LIB_EXT}
PLUGINS=${PLUGIN_OUT}/libplugin.${LIB_EXT}
LOG_PLUGIN=${PLUGIN_OUT}/${LIB_PREFIX}logplugin.${LIB_EXT}
COMMON_PLUGINS=${PLUGIN_OUT}/${LIB_PREFIX}envplugin.${LIB_EXT} ${PLUGIN_OUT}/${LIB_PREFIX}cpuplugin.${LIB_EXT} ${PLUGIN_OUT}/${LIB_PREFIX}memoryplugin.${LIB_EXT}#${PLUGIN_OUT}/libosplugin.${LIB_EXT}
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

java: setup res common ${JAVA_AGENT} ${CONNECTORS} ${COMMON_PLUGINS} #${PLUGINS}
	@echo "JAVA build complete"

omr: HC_OUT=${OMR_OUT}
omr: setup common ${OMR_AGENT} ${CONNECTORS} ${PLUGINS}
	@echo "omr build complete"

node: HC_OUT=${NODE_OUT}
node: setup common ${NODE_ADDON} ${CONNECTORS} ${COMMON_PLUGINS} ${NODE_PLUGINS}
	@echo "node build complete"

python: HC_OUT=${PYTHON_OUT}
python: setup common ${CORE_AGENT} ${PYTHON_AGENT} ${CONNECTORS} ${COMMON_PLUGINS}
		@echo "python build complete"

ruby: HC_OUT=${RUBY_OUT}
ruby: setup common ${CORE_AGENT} ${RUBY_AGENT} ${CONNECTORS} ${COMMON_PLUGINS}
		@echo "ruby build complete"

pyplug: ${PYPLUG}

test: setup common ${CONNECTORS} ${AGENT} ${TEST}
	@echo "Test build complete"

#core: HC_OUT=${CORE_OUT}
core: setup res common ${CORE_AGENT} ${CONNECTORS} ${ENVPLUGIN_OBJS} ${CPUPLUGIN_OBJS} ${MEMPLUGIN_OBJS} ${COMMON_PLUGINS}
	@echo "Core build complete"

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

${PLUGIN_OUT}/${LIB_PREFIX}nodegcplugin.${LIB_EXT}: ${NODEGC_OBJS}
	${LINK} ${LINK_OPT} ${LIBFLAGS} ${LIB_OBJOPT} "$<" ${LIBPATH}"${HC_OUT}" ${HC_LIB_USE} ${EXELIBS} ${NODEEXELIBS}
	@echo "NodeGC lib built"

${PLUGIN_OUT}/${LIB_PREFIX}nodeprofplugin.${LIB_EXT}: ${NODEPROF_OBJS}
	${LINK} ${LINK_OPT} ${LIBFLAGS} ${LIB_OBJOPT} "$<" ${LIBPATH}"${HC_OUT}" ${HC_LIB_USE} ${EXELIBS} ${NODEEXELIBS}
	@echo "NodeProf lib built"

${PLUGIN_OUT}/${LIB_PREFIX}nodeenvplugin.${LIB_EXT}: ${NODEENV_OBJS}
	${LINK} ${LINK_OPT} ${LIBFLAGS} ${LIB_OBJOPT} "$<" ${LIBPATH}"${HC_OUT}" ${HC_LIB_USE} ${EXELIBS} ${NODEEXELIBS}
	@echo "NodeEnv lib built"

${PLUGIN_OUT}/${LIB_PREFIX}envplugin.${LIB_EXT}: ${ENVPLUGIN_OBJS}
	${LINK} ${LINK_PLUG} ${LIBFLAGS} ${LIB_OBJOPT} ${ENVPLUGIN_OBJS} ${EXELIBS}
	@echo "Environment lib built"

${PLUGIN_OUT}/${LIB_PREFIX}memoryplugin.${LIB_EXT}: ${MEMPLUGIN_OBJS}
	${LINK} ${LINK_PLUG} ${LIBFLAGS} ${LIB_OBJOPT} ${MEMPLUGIN_OBJS} ${EXELIBS}
	@echo "Memory lib built"

${PLUGIN_OUT}/libpyplugin.${LIB_EXT}: ${PYPLUGIN_OBJS}
	${LINK} ${LDFLAGS} ${LIBFLAGS} ${OBJOPT} ${PYPLUGIN_OBJS} ${COMMON_OBJS}
	@echo "Python plugin lib built"

${CONNECTOR_OUT}/${LIB_PREFIX}hcmqtt.${LIB_EXT}: ${MQTT_CONNECTOR_OBJS} ${PAHO_ASYNC_OBJS} ${COMMON_OBJS}
	${LINK} ${LINK_OPT} ${LIBFLAGS} ${LIB_OBJOPT} ${MQTT_CONNECTOR_OBJS} ${PAHO_ASYNC_OBJS} ${MQTT_LIB_OPTIONS} ${LD_OPT} ${EXELIBS} ${EXEFLAGS}
	@echo "MQTT connector lib built"

${CONNECTOR_OUT}/${LIB_PREFIX}hcapiplugin.${LIB_EXT}: ${API_CONNECTOR_OBJS} ${COMMON_OBJS}
	${LINK} ${LINK_OPT} ${LIBFLAGS} ${LIB_OBJOPT} ${API_CONNECTOR_OBJS} ${LD_OPT} ${EXELIBS} ${EXEFLAGS}
	@echo "InProcess connector lib built"

${CONNECTOR_OUT}/libostream.${LIB_EXT}: ${OSTREAM_CONNECTOR_OBJS}
	${LINK} ${LINK_OPT} ${LIBPATH}"${AGENT_OUT}" ${LIBFLAGS} ${LIBPATH}"${HC_OUT}" ${HC_LIB_USE} ${LIB_OBJOPT} ${OSTREAM_CONNECTOR_OBJS} ${EXELIBS} ${EXEFLAGS}
	@echo "OStream connector lib built"

${JAVA_OUT}/${LIB_PREFIX}javametrics.${LIB_EXT}: ${AGENT_OBJS} ${JAVA_OBJS}
	${LINK} ${LINK_OPT} ${LIBFLAGS} ${LIB_OBJOPT} ${JAVA_OBJS} ${AGENT_OBJS} ${LD_OPT}  ${EXELIBS} ${EXEFLAGS}
	@echo "JAVAMETRICS lib built"


${JAVA_OUT}/${LIB_PREFIX}agentcore.${LIB_EXT}: ${AGENT_OBJS}
	${LINK} ${LINK_OPT}  ${LIBFLAGS} ${LIB_OBJOPT} ${AGENT_OBJS} ${LD_OPT} ${EXELIBS} ${EXEFLAGS}
	@echo "core Healthcenter lib built"

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
	${CC} ${INCS} ${CFLAGS} -I"./connector/mqtt" -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/agent/Agent.cpp

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

${CONNECTOR_OUT}/OStreamConnector.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/connector/ostream/OStreamConnector.cpp

${CONNECTOR_OUT}/HLConnectorPlugin.o:
	${CC} ${INCS} -I${JAVA_PLAT_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/connector/headless/HLConnectorPlugin.cpp

${CONNECTOR_OUT}/HLConnector.o:
	${CC} ${INCS} -I${JAVA_PLAT_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/connector/headless/HLConnector.cpp

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

${COMMON_OUT}/Logger.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/common/Logger.cpp

${COMMON_OUT}/LogManager.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/common/LogManager.cpp

${COMMON_OUT}/zos_switch_from_ifa.o:
	${ASM} ${SRC}/common/port/${PORTDIR}/zos_switch_from_ifa.s

${COMMON_OUT}/zos_switch_to_ifa.o:
	${ASM} ${SRC}/common/port/${PORTDIR}/zos_switch_to_ifa.s

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

${PLUGIN_OUT}/LegacyData.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/j9/jmx/os/legacy/LegacyData.cpp

${PLUGIN_OUT}/JMXSourceManager.o:
	${CC} ${INCS} ${CFLAGS} -I${JAVA_PLAT_INCLUDE} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/j9/jmx/JMXSourceManager.cpp

${PLUGIN_OUT}/JMXPullSource.o:
	${CC} ${INCS} ${CFLAGS} -I${JAVA_PLAT_INCLUDE} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/j9/jmx/JMXPullSource.cpp

${PLUGIN_OUT}/JMXUtility.o:
	${CC} ${INCS} ${CFLAGS} -I${JAVA_PLAT_INCLUDE} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/j9/jmx/JMXUtility.cpp

${COMMON_OUT}/memUtils.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/common/util/memUtils.cpp

${COMMON_OUT}/MemoryManager.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/common/MemoryManager.cpp

${COMMON_OUT}/strUtils.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/common/util/strUtils.cpp

${COMMON_OUT}/sysUtils.o:
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/common/util/sysUtils.cpp

${PLUGIN_OUT}/ENVMXBean.o:
	${CC} ${INCS} ${CFLAGS} -I${JAVA_PLAT_INCLUDE} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/j9/jni/env/ENVMXBean.cpp

${PLUGIN_OUT}/ThreadDataProvider.o:
	${CC} ${INCS} ${CFLAGS} -I${JAVA_PLAT_INCLUDE} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/j9/jni/threads/ThreadDataProvider.cpp

${PLUGIN_OUT}/MemoryDataProvider.o:
	${CC} ${INCS} ${CFLAGS} -I${JAVA_PLAT_INCLUDE} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/j9/jni/memory/MemoryDataProvider.cpp

${PLUGIN_OUT}/MemoryCounterDataProvider.o:
	${CC} ${INCS} ${CFLAGS} -I${JAVA_PLAT_INCLUDE} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/j9/jni/memorycounter/MemoryCounterDataProvider.cpp

${PLUGIN_OUT}/pyplugin.o:
	${CC} ${INCS} -I${OMR_SRC_INCLUDE} ${PYTHON_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/pytest/pyplugin.cpp

${PLUGIN_OUT}/nodegcplugin.o: ${SRC}/monitoring/plugins/nodegc/nodegcplugin.cpp
	${CC} ${INCS} -I${NODE_SDK_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/nodegc/nodegcplugin.cpp

${PLUGIN_OUT}/nodeprofplugin.o: ${SRC}/monitoring/plugins/nodeprof/nodeprofplugin.cpp
	${CC} ${INCS} -I${NODE_SDK_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/nodeprof/nodeprofplugin.cpp

${PLUGIN_OUT}/nodeenvplugin.o: ${SRC}/monitoring/plugins/nodeenv/nodeenvplugin.cpp
	${CC} ${INCS} -I${NODE_SDK_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/nodeenv/nodeenvplugin.cpp

${PLUGIN_OUT}/envplugin.o: ${SRC}/monitoring/plugins/common/environment/envplugin.cpp
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/common/environment/envplugin.cpp

${PLUGIN_OUT}/cpuplugin.o: ${SRC}/monitoring/plugins/common/cpu/cpuplugin.cpp
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/common/cpu/cpuplugin.cpp

${PLUGIN_OUT}/MemoryPlugin.o: ${SRC}/monitoring/plugins/common/memory/MemoryPlugin.cpp
	${CC} ${INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/common/memory/MemoryPlugin.cpp

#-------------------------------------------------------------------------------------------
#JAVA vm files which make up various JAVA shim levels
#-------------------------------------------------------------------------------------------

${JAVA_OUT}/healthcenter.o:
	${CC} ${INCS} -I${JAVA_PLAT_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/vm/java/healthcenter.cpp

${JAVA_OUT}/JVMTIMemoryManager.o:
	${CC} ${INCS} -I${JAVA_PLAT_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/vm/java/JVMTIMemoryManager.cpp

${JAVA_OUT}/ClassHistogramProvider.o:
	${CC} ${INCS} -I${JAVA_PLAT_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/j9/ClassHistogramProvider.cpp

${JAVA_OUT}/EnvironmentPlugin.o:
	${CC} ${INCS} -I${JAVA_PLAT_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/j9/environment/EnvironmentPlugin.cpp

${JAVA_OUT}/LockingPlugin.o:
	${CC} ${INCS} -I${JAVA_PLAT_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/j9/locking/LockingPlugin.cpp

${JAVA_OUT}/ThreadsPlugin.o:
	${CC} ${INCS} -I${JAVA_PLAT_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/j9/threads/ThreadsPlugin.cpp

${JAVA_OUT}/MemoryPlugin.o:
	${CC} ${INCS} -I${JAVA_PLAT_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/j9/memory/MemoryPlugin.cpp

${JAVA_OUT}/MemCountersPlugin.o:
	${CC} ${INCS} -I${JAVA_PLAT_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/j9/memorycounters/MemCountersPlugin.cpp

${JAVA_OUT}/CpuPlugin.o:
	${CC} ${INCS} -I${JAVA_PLAT_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/j9/cpu/CpuPlugin.cpp

${JAVA_OUT}/AppPlugin.o:
	${CC} ${INCS} -I${JAVA_PLAT_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/j9/api/AppPlugin.cpp

${JAVA_OUT}/Util.o:
	${CC} ${INCS} -I${JAVA_PLAT_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/j9/Util.cpp

${JAVA_OUT}/TraceDataProvider.o:
	${CC} ${INCS} -I${JAVA_PLAT_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/j9/trace/TraceDataProvider.cpp

${JAVA_OUT}/TraceReceiver.o:
	${CC} ${INCS} -I${JAVA_PLAT_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/j9/trace/TraceReceiver.cpp

${JAVA_OUT}/MethodLookupProvider.o:
	${CC} ${INCS} -I${JAVA_PLAT_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/j9/methods/MethodLookupProvider.cpp

${JAVA_OUT}/DumpHandler.o:
	${CC} ${INCS} -I${JAVA_PLAT_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/j9/DumpHandler.cpp

${JAVA_OUT}/JMXConnectorPlugin.o:
	${CC} ${INCS} -I${JAVA_PLAT_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/connector/jmx/JMXConnectorPlugin.cpp

${JAVA_OUT}/JMXConnector.o:
	${CC} ${INCS} -I${JAVA_PLAT_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/connector/jmx/JMXConnector.cpp


#---------------
# MQTT Connector
#---------------
#${CONNECTOR_OUT}/MQTTConnector.o: HC_EXPORT=
${CONNECTOR_OUT}/MQTTConnector.o:
	${GCC} ${INCS} ${MQTT_INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/connector/mqtt/MQTTConnector.cpp

#----------------------
# PAHO MQTT client code
#----------------------
${PAHO_OUT}/Clients.o:
	${GCC} ${INCS} ${MQTT_INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${PAHO_SRC}/Clients.c

${PAHO_OUT}/Heap.o:
	${GCC} ${INCS} ${MQTT_INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${PAHO_SRC}/Heap.c

${PAHO_OUT}/LinkedList.o:
	${GCC} ${INCS} ${MQTT_INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${PAHO_SRC}/LinkedList.c

${PAHO_OUT}/Log.o:
	${GCC} ${INCS} ${MQTT_INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${PAHO_SRC}/Log.c

${PAHO_OUT}/Messages.o:
	${GCC} ${INCS} ${MQTT_INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${PAHO_SRC}/Messages.c

${PAHO_OUT}/MQTTAsync.o:
	${GCC} ${INCS} ${MQTT_INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${PAHO_SRC}/MQTTAsync.c

${PAHO_OUT}/MQTTPacket.o:
	${GCC} ${INCS} ${MQTT_INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${PAHO_SRC}/MQTTPacket.c

${PAHO_OUT}/MQTTPacketOut.o:
	${GCC} ${INCS} ${MQTT_INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${PAHO_SRC}/MQTTPacketOut.c

${PAHO_OUT}/MQTTPersistence.o:
	${GCC} ${INCS} ${MQTT_INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${PAHO_SRC}/MQTTPersistence.c

${PAHO_OUT}/MQTTPersistenceDefault.o:
	${GCC} ${INCS} ${MQTT_INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${PAHO_SRC}/MQTTPersistenceDefault.c

${PAHO_OUT}/MQTTProtocolClient.o:
	${GCC} ${INCS} ${MQTT_INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${PAHO_SRC}/MQTTProtocolClient.c

${PAHO_OUT}/MQTTProtocolOut.o:
	${GCC} ${INCS} ${MQTT_INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${PAHO_SRC}/MQTTProtocolOut.c

${PAHO_OUT}/SocketBuffer.o:
	${GCC} ${INCS} ${MQTT_INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${PAHO_SRC}/SocketBuffer.c

${PAHO_OUT}/Socket.o:
	${GCC} ${INCS} ${MQTT_INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${PAHO_SRC}/Socket.c

${PAHO_OUT}/StackTrace.o:
	${GCC} ${INCS} ${MQTT_INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${PAHO_SRC}/StackTrace.c

${PAHO_OUT}/Thread.o:
	${GCC} ${INCS} ${MQTT_INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${PAHO_SRC}/Thread.c

${PAHO_OUT}/Tree.o:
	${GCC} ${INCS} ${MQTT_INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${PAHO_SRC}/Tree.c

${PAHO_OUT}/utf-8.o:
	${GCC} ${INCS} ${MQTT_INCS} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${PAHO_SRC}/utf-8.c



#-------------------------------------------------------------------------------------------
#OMR vm files which make up various OMR shim levels
#-------------------------------------------------------------------------------------------

# OMR

${OMR_OUT}/healthcenter.o:
	${CC} ${INCS} -I${OMR_SRC_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/vm/omr/healthcenter.cpp

${OMR_OUT}/CpuDataProvider.o:
	${CC} ${INCS} -I${OMR_SRC_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/omr/CpuDataProvider.cpp

${OMR_OUT}/NativeMemoryDataProvider.o:
	${CC} ${INCS} -I${OMR_SRC_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/omr/NativeMemoryDataProvider.cpp

${OMR_OUT}/MethodLookupProvider.o:
	${CC} ${INCS} -I${OMR_SRC_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/omr/MethodLookupProvider.cpp

${OMR_OUT}/TraceDataProvider.o:
	${CC} ${INCS} -I${OMR_SRC_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/omr/TraceDataProvider.cpp

${OMR_OUT}/MemoryCountersDataProvider.o:
	${CC} ${INCS} -I${OMR_SRC_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/monitoring/plugins/omr/MemoryCountersDataProvider.cpp


#-------------------------------------------------------------------------------------------
#Node vm files which make up various Node shim levels
#-------------------------------------------------------------------------------------------

${NODE_OUT}/${LIB_PREFIX}nodecon.${LIB_EXT}: ${NODE_OUT}/nodeconnector.o
	${LINK} ${LINK_OPT} ${LIBFLAGS} ${LIB_OBJOPT} ${NODE_OUT}/nodeconnector.o ${LIBPATH}"${HC_OUT}" ${HC_LIB_USE} ${NODEEXELIBS} ${EXELIBS}

${NODE_OUT}/nodeconnector.o:
	${CC} ${INCS} -I${NODE_SDK_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/vm/node/nodeconnector.cpp

#-------------------------------------------------------------------------------------------
#Python vm files which make up various Python shim levels
#-------------------------------------------------------------------------------------------

${PYTHON_OUT}/healthcenter.o:
	${CC} ${INCS} -I${PYTHON_SRC_INCLUDE} ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/vm/python/healthcenter.cpp

#-------------------------------------------------------------------------------------------
#Ruby vm files which make up various Ruby shim levels
#-------------------------------------------------------------------------------------------

${RUBY_OUT}/healthcenter.o:
	${CC} ${INCS} -I${RUBY_SRC_INCLUDE} -I${RUBY_SRC_INCLUDE}/x86_64-linux ${CFLAGS} -D${PLATFORM} ${OBJOPT} ${SRC}/vm/ruby/healthcenter.cpp

#-------------------------------------------------------------------------------------------


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
	cp src/properties/java/healthcenter.properties ${INSTALL_DIR}
	cp ${JAVA_OUT}/${LIB_PREFIX}javametrics.${LIB_EXT} ${INSTALL_DIR}
	cp ${PLUGIN_OUT}/*.${LIB_EXT} ${INSTALL_DIR}/plugins
	@echo "-----------------------------------------------------------------------------------------------------------------------"
