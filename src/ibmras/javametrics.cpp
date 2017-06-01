/**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */
#if defined(_ZOS)
#define _UNIX03_SOURCE
#endif
#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "ibmras/common/logging.h"
#include "ibmras/monitoring/agent/Agent.h"
#include "ibmras/monitoring/AgentExtensions.h"
#include "ibmras/monitoring/Typesdef.h"
#include "ibmras/monitoring/plugins/j9/trace/TraceDataProvider.h"
#include "ibmras/monitoring/plugins/j9/api/AppPlugin.h"
#include "ibmras/monitoring/plugins/j9/methods/MethodLookupProvider.h"
#include "ibmras/monitoring/plugins/j9/DumpHandler.h"
#include "ibmras/monitoring/plugins/j9/ClassHistogramProvider.h"
#include "ibmras/monitoring/connector/headless/HLConnectorPlugin.h"
#include "javametrics.h"
#include "ibmras/common/Properties.h"
#include "ibmras/common/util/strUtils.h"
#include "ibmras/common/port/Process.h"
#include "ibmras/vm/java/JVMTIMemoryManager.h"
#include "ibmras/monitoring/plugins/j9/Util.h"

#include "ibmras/monitoring/plugins/j9/environment/EnvironmentPlugin.h"
#include "ibmras/monitoring/plugins/j9/locking/LockingPlugin.h"
#include "ibmras/monitoring/plugins/j9/threads/ThreadsPlugin.h"
#include "ibmras/monitoring/plugins/j9/memory/MemoryPlugin.h"
#include "ibmras/monitoring/plugins/j9/memorycounters/MemCountersPlugin.h"
#include "ibmras/monitoring/plugins/j9/cpu/CpuPlugin.h"

struct __jdata;

#include "jvmti.h"
#include "jni.h"
#include "ibmjvmti.h"
#include "jni.h"
#include "jniport.h"

#if defined(_WINDOWS)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

/*########################################################################################################################*/
/*########################################################################################################################*/
/*########################################################################################################################*/
static const char* JAVAMETRICS_PROPERTIES_PREFIX =
		"com.ibm.javametrics.";

int launchAgent();
void initialiseProperties(const std::string &options);
void addPlugins();
std::string agentOptions;
ibmras::common::Properties hcprops;
static JavaVM *theVM;
static jobject api_callback = NULL;

jvmFunctions tDPP;

/*function holders for api connector */
void (*registerListener)(void (*)(const char *, unsigned int, void*));
void (*deregisterListener)();
void (*sendControl)(const char*, unsigned int, void*);

jvmtiEnv *pti = NULL;

typedef struct __jdata jdata_t;

/* ensure common reporting of JNI version required */
#define JNI_VERSION JNI_VERSION_1_4

jint initialiseAgent(JavaVM *vm, char *options, void *reserved, int onAttach);

static bool agentStarted = false;

IBMRAS_DEFINE_LOGGER("java");


ibmras::monitoring::agent::Agent* agent;

/* ======================= */
/* Agent control functions */
/* ======================= */
/******************************/
extern "C" JNIEXPORT void JNICALL
cbVMInit(jvmtiEnv *jvmti_env, JNIEnv* jni_env, jthread thread) {
	initialiseProperties(agentOptions);
	agent->init();
	launchAgent();
}
/******************************/
extern "C" JNIEXPORT void JNICALL
cbVMDeath(jvmtiEnv *jvmti_env, JNIEnv* jni_env) {
	IBMRAS_DEBUG(debug, "VmDeath event");
	agent->stop();
	agent->shutdown();
}
/******************************/
JNIEXPORT void JNICALL
Agent_OnUnload(JavaVM *vm) {
	IBMRAS_DEBUG(debug, "OnUnload");
}

/******************************/
JNIEXPORT jint JNICALL
Agent_OnAttach(JavaVM *vm, char *options, void *reserved) {

	jint rc = 0;
	IBMRAS_DEBUG(debug, "> Agent_OnAttach");if (!agentStarted) {
		rc = initialiseAgent(vm, options, reserved, 1);
		initialiseProperties(agentOptions);
		agent->init();
		agentStarted=true;
	} else {
		initialiseProperties(agentOptions);
	}
	rc = launchAgent();
	IBMRAS_DEBUG_1(debug,
			"< Agent_OnAttach. rc=%d", rc);
	return rc;
}

/******************************/
JNIEXPORT jint JNICALL
Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
	IBMRAS_DEBUG(debug, "OnLoad");
	jint rc = 0;
	if (!agentStarted) {
		rc = initialiseAgent(vm, options, reserved, 0);
		agentStarted=true;
	}

	IBMRAS_DEBUG_1(debug, "< Agent_OnLoad. rc=%d",
			rc); return rc;
}

/****************************/
jint initialiseAgent(JavaVM *vm, char *options, void *reserved, int onAttach) {
	jvmtiCapabilities cap;
	jvmtiEventCallbacks cb;

	jint rc, i, j;

	jint xcnt;
	jvmtiExtensionFunctionInfo * exfn;
	jvmtiExtensionEventInfo * exev;

	jvmtiExtensionFunctionInfo * fi;
	jvmtiExtensionEventInfo * ei;
	jvmtiParamInfo * pi;

	theVM = vm;
	tDPP.theVM = vm;
	if (options == NULL) {
		agentOptions = "";
	} else {
		agentOptions = options;
	}

	vm->GetEnv((void **) &pti, JVMTI_VERSION_1);

	ibmras::common::memory::setDefaultMemoryManager(
			new ibmras::vm::java::JVMTIMemoryManager(pti));

	rc = pti->GetExtensionFunctions(&xcnt, &exfn);

	if (JVMTI_ERROR_NONE != rc) {
		IBMRAS_DEBUG_1(debug, "GetExtensionFunctions: rc = %d", rc);
	}

	/* Cleanup after GetExtensionFunctions while extracting information */
	tDPP.pti = pti;

#if defined(_ZOS)
#pragma convert("ISO8859-1")
#endif
	fi = exfn;
	pti->Deallocate((unsigned char *) exfn);

	/*--------------------------------------
	 Manage Extension Events
	 -------------------------------------*/

	rc = pti->GetExtensionEvents(&xcnt, &exev);

	/* Cleanup after GetExtensionEvents while extracting information */

	ei = exev;

	for (i = 0; i < xcnt; i++) {

		/* Cleanup */

		pi = ei->params;

		for (j = 0; j < ei->param_count; j++) {
			pti->Deallocate((unsigned char*) pi->name);

			pi++;
		}
		pti->Deallocate((unsigned char*) ei->id);
		pti->Deallocate((unsigned char*) ei->short_description);
		pti->Deallocate((unsigned char *) ei->params);

		ei++;
	}
	pti->Deallocate((unsigned char *) exev);

	memset(&cb, 0, sizeof(cb));

	cb.VMInit = cbVMInit;
	cb.VMDeath = cbVMDeath;

	pti->SetEventCallbacks(&cb, sizeof(cb));
	pti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, NULL);
	pti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_DEATH, NULL);

	addPlugins();

	IBMRAS_DEBUG_1(debug, "< initialiseAgent rc=%d", rc);
	return rc;
}

int ExceptionCheck(JNIEnv *env) {
	if (env->ExceptionCheck()) {
		IBMRAS_DEBUG(debug, "JNI exception:");
		env->ExceptionDescribe();
		env->ExceptionClear();
		return 1;
	} else {
		return 0;
	}
}

void getHCProperties(const std::string &options) {

	JNIEnv *ourEnv = NULL;

	jint rc = theVM->GetEnv((void **) &ourEnv, JNI_VERSION);
	if (rc < 0 || NULL == ourEnv) {
		IBMRAS_DEBUG(warning, "getEnv failed");
		return;
	}

	IBMRAS_DEBUG(debug, "Calling FindClass");
#if defined(_ZOS)
#pragma convert("ISO8859-1")
#endif
	jclass hcoptsClass =
			ourEnv->FindClass(
					"com/ibm/java/diagnostics/healthcenter/agent/mbean/HealthCenterOptionHandler");
#if defined(_ZOS)
#pragma convert(pop)
#endif
	if (ExceptionCheck(ourEnv) || hcoptsClass == NULL) {
		IBMRAS_DEBUG(warning, "could not find HealthCenterOptionHandler")
		return;
	}
	IBMRAS_DEBUG(debug, "Calling GetStaticMethodID");
#if defined(_ZOS)
#pragma convert("ISO8859-1")
#endif
	jmethodID getPropertiesMethod = ourEnv->GetStaticMethodID(hcoptsClass,
			"getProperties", "([Ljava/lang/String;)[Ljava/lang/String;");
#if defined(_ZOS)
#pragma convert(pop)
#endif
	if (ExceptionCheck(ourEnv) || getPropertiesMethod == NULL) {
		IBMRAS_DEBUG(warning, "could not find getProperties method")
		return;
	}

	std::stringstream ss;
	ss << ibmras::common::port::getProcessId();
	std::string pid = ss.str();
	jobjectArray applicationArgs = NULL;

#if defined(_ZOS)
	char* pidStr = ibmras::common::util::createAsciiString(pid.c_str());
#else
	const char* pidStr = pid.c_str();
#endif

#if defined(_ZOS)
#pragma convert("ISO8859-1")
#endif
	jstring pidArg = ourEnv->NewStringUTF(pidStr);
	if (!ExceptionCheck(ourEnv)) {

		jstring opts = ourEnv->NewStringUTF(options.c_str());
		if (!ExceptionCheck(ourEnv)) {

			applicationArgs = ourEnv->NewObjectArray(2,
					ourEnv->FindClass("java/lang/String"), NULL);

			if (!ExceptionCheck(ourEnv)) {
				ourEnv->SetObjectArrayElement(applicationArgs, 0, pidArg);
				if (!ExceptionCheck(ourEnv)) {
					ourEnv->SetObjectArrayElement(applicationArgs, 1, opts);
					if (ExceptionCheck(ourEnv)) {
						applicationArgs = NULL;
					}
				} else {
					applicationArgs = NULL;
				}
			}
			ourEnv->DeleteLocalRef(opts);
		}
		ourEnv->DeleteLocalRef(pidArg);
	}

	jobjectArray hcprops = (jobjectArray) ourEnv->CallStaticObjectMethod(
			hcoptsClass, getPropertiesMethod, applicationArgs);

#if defined(_ZOS)
#pragma convert(pop)
#endif
#if defined(_ZOS)
	ibmras::common::memory::deallocate((unsigned char**)&pidStr);
#endif

	if (ExceptionCheck(ourEnv) || hcprops == NULL) {
		IBMRAS_DEBUG(warning, "No healthcenter.properties found")
		return;
	}

	jsize numProps = ourEnv->GetArrayLength(hcprops);
	IBMRAS_DEBUG_1(debug, "%d.properties found", numProps);

	ibmras::common::Properties theProps;

	for (jsize i = 0; i < numProps; ++i) {
		jstring line = (jstring) ourEnv->GetObjectArrayElement(hcprops, i);
		const char* lineUTFChars = ourEnv->GetStringUTFChars(line, NULL);
#if defined(_ZOS)
		char* lineChars = ibmras::common::util::createNativeString(lineUTFChars);
#else
		const char* lineChars = lineUTFChars;
#endif
		if (lineChars) {
			std::string lineStr(lineChars);
			size_t pos = lineStr.find('=');
			if ((pos != std::string::npos) && (pos < lineStr.size())) {
				std::string key(lineStr.substr(0, pos));
				std::string value(lineStr.substr(pos + 1));
				theProps.put(key, value);

			}
		}

		ourEnv->ReleaseStringUTFChars(line, lineUTFChars);
#if defined(_ZOS)
		ibmras::common::memory::deallocate((unsigned char**)&lineChars);
#endif

	}

	std::string agentPropertyPrefix = agent->getAgentPropertyPrefix();
	std::list < std::string > hcPropKeys = theProps.getKeys(
			JAVAMETRICS_PROPERTIES_PREFIX);
	for (std::list<std::string>::iterator i = hcPropKeys.begin();
			i != hcPropKeys.end(); ++i) {
		std::string key = i->substr(strlen(JAVAMETRICS_PROPERTIES_PREFIX));
		if (key.length() > 0) {
			std::string newKey = agentPropertyPrefix + key;
			if (!theProps.exists(newKey)) {
				theProps.put(newKey, theProps.get(*i));
			}
		}
	}
	agent->setProperties(theProps);
}

std::string setAgentLibPathAIX() {

#if defined(_64BIT) 
	return agent->getProperty("java.home")+"/lib/ppc64";
#else
	return agent->getProperty("java.home") + "/lib/ppc";
#endif

}

std::string setAgentLibPathZOS() {

#if defined(_64BIT)
	return agent->getProperty("java.home")+"/lib/s390x";
#else
	return agent->getProperty("java.home") + "/lib/s390";
#endif
}



static std::string fileJoin(const std::string& path,
		const std::string& filename) {
#if defined(_WINDOWS)
	static const std::string fileSeparator("\\");
#else
	static const std::string fileSeparator("/");
#endif
	return path + fileSeparator + filename;
}

#if defined(_WINDOWS)
void* getApiFunc(std::string pluginPath, std::string funcName) {
	std::string apiPlugin = fileJoin(pluginPath, "hcapiplugin.dll");
	HMODULE handle = LoadLibrary(apiPlugin.c_str());
	if (handle == NULL) {
		std::cerr << "API Connector Listener: failed to open hcapiplugin.dll \n";
		return NULL;
	}
	FARPROC apiFunc = GetProcAddress(handle, const_cast<char *>(funcName.c_str()));
	if (apiFunc == NULL) {
		std::cerr << "API Connector Listener: cannot find symbol '" << funcName << " in hcapiplugin.dll \n";
		return NULL;
	}
	return (void*) apiFunc;
}
#else
void* getApiFunc(std::string pluginPath, std::string funcName) {
#if defined(__MACH__) || defined(__APPLE__)
	std::string libname = "libhcapiplugin.dylib";
#else
	std::string libname = "libhcapiplugin.so";
#endif
	std::string apiPlugin = fileJoin(pluginPath, libname);
	void* handle = dlopen(apiPlugin.c_str(), RTLD_LAZY);
	if (!handle) {
		std::cerr << "API Connector Listener: failed to open " << libname
				<< ": " << dlerror() << "\n";
		return NULL;
	}
	void* apiFunc = dlsym(handle, funcName.c_str());
	if (!apiFunc) {
		std::cerr << "API Connector Listener: cannot find symbol '" << funcName
				<< "' in " << libname << ": " << dlerror() << "\n";
		dlclose(handle);
		return NULL;
	}
	return apiFunc;
}
#endif

void addAPIPlugin() {

	agent = ibmras::monitoring::agent::Agent::getInstance();

	std::string agentLibPath =
			ibmras::common::util::LibraryUtils::getLibraryDir(
					"javametrics.dll", (void*) launchAgent);

	if (agentLibPath.length() == 0) {
		agentLibPath = agent->getProperty("com.ibm.system.agent.path");
	}

//If the agentLibPath is still empty, set the required path depending on the operating system
	if (agentLibPath.length() == 0) {

#if defined(_AIX)
		agentLibPath = setAgentLibPathAIX();
#elif defined(_ZOS)
		agentLibPath = setAgentLibPathZOS();
#endif

	}

//if we have a remote agent we want to change the agentLibPath here
	std::string agentRemotePath = agent->getProperty(
			"com.ibm.diagnostics.healthcenter.agent.path");
	if (agentRemotePath.length() != 0) {
		std::size_t libPos = agentLibPath.find("/lib");
		std::string relativeLibPath = agentLibPath.substr(libPos);
		agentLibPath = agentRemotePath + relativeLibPath;
	}

	agent->addPlugin(agentLibPath, "hcapiplugin");

	registerListener =
			(void (*)(
					void (*func)(const char*, unsigned int,
							void*))) getApiFunc(agentLibPath, std::string("registerListener"));deregisterListener
	= (void (*)())getApiFunc(agentLibPath, std::string("deregisterListener"));sendControl
	= (void (*)(const char*, unsigned int,
			void*)) getApiFunc(agentLibPath, std::string("sendControl"));

} 

void addPlugins() {
	agent = ibmras::monitoring::agent::Agent::getInstance();
// AIX and z/OS can't load the MQTT or API plugins here, as it needs the Java system
// properties from an initialised VM, so needs to wait until cbVMInit has been called.
#if defined(_AIX) || defined(_ZOS)
#else
	addAPIPlugin();
#endif

	if (tDPP.pti == NULL) {
		IBMRAS_DEBUG(debug, "tDPP.pti is null");
	}

	IBMRAS_DEBUG(debug, "Adding plugins");

	agent->addPlugin(
			ibmras::monitoring::plugins::j9::api::AppPlugin::getInstance(tDPP));

	ibmras::monitoring::Plugin* environment =
			ibmras::monitoring::plugins::j9::environment::EnvironmentPlugin::getPlugin(
					&tDPP);
}

void initialiseProperties(const std::string &options) {
	agent = ibmras::monitoring::agent::Agent::getInstance();
	agent->setAgentProperty("launch.options", options);
	//getHCProperties(options);
	agent->setLogLevels();

}
/**
 * launch agent code
 */
int launchAgent() {

	agent = ibmras::monitoring::agent::Agent::getInstance();

	if (agent->isHeadlessRunning()) {
		return -2;
	}

	agent->setLogLevels();

// now we have the system properties, AIX and z/OS can load the MQTT and API plugins.
#if defined(_AIX) || defined(_ZOS)
	addAPIPlugin();
#endif

	std::string agentVersion = agent->getVersion();
	IBMRAS_LOG_1(fine, "Health Center Agent %s", agentVersion.c_str());
	// Set connector properties based on data.collection.level

	agent->start();

	return 0;
}


void sendMsg(const char *sourceId, uint32 size, void *data) {
	bool attachFlag = false;

	if (theVM == NULL) {
		IBMRAS_DEBUG(warning, "No VM");
		return;
	}
	if (api_callback == NULL) {
		IBMRAS_DEBUG(warning, "No Callback");
		return;
	}

	JNIEnv *ourEnv = NULL;

	jint rc = theVM->GetEnv((void **) &ourEnv, JNI_VERSION);
	if (rc == JNI_EDETACHED) {
		rc = ibmras::monitoring::plugins::j9::setEnv(&ourEnv,
				"Health Center (healthcenter)", theVM, false);
		attachFlag = true;
	}
	if (rc < 0 || NULL == ourEnv) {
		IBMRAS_DEBUG(warning, "sendMsg:getEnv failed");
		return;
	}

	jclass cls = ourEnv->GetObjectClass(api_callback);
#if defined(_ZOS)
#pragma convert("ISO8859-1")
#endif
	jmethodID mid = ourEnv->GetMethodID(cls, "receiveData",
			"(Ljava/lang/String;[B)V");
#if defined(_ZOS)
#pragma convert(pop)
#endif
	jbyteArray arr = ourEnv->NewByteArray(size);
	ourEnv->SetByteArrayRegion(arr, 0, size, (jbyte*) data);
	ourEnv->CallVoidMethod(api_callback, mid, ourEnv->NewStringUTF(sourceId),
			arr);
	if (attachFlag) {
		theVM->DetachCurrentThread();
	}
}

extern "C" {
JNIEXPORT void JNICALL
Java_websockets_JavametricsWebSocket_regListener(JNIEnv *env, jclass clazz, jobject obj) {
	api_callback = env->NewGlobalRef(obj);
	registerListener(&sendMsg);
}

JNIEXPORT void JNICALL
Java_websockets_JavametricsWebSocket_deregListener(JNIEnv *env, jobject obj) {
	deregisterListener();
}

JNIEXPORT void JNICALL
Java_websockets_JavametricsWebSocket_sendMessage(JNIEnv *env, jobject obj, jstring topic, jbyteArray ident) {

	const char *s = env->GetStringUTFChars(topic,NULL);
	if (s) {
		jboolean isCopy;
		jbyte* i = env->GetByteArrayElements(ident, &isCopy);
		sendControl(s, env->GetArrayLength(ident), (void *)i);
		env->ReleaseStringUTFChars(topic,s);
		env->ReleaseByteArrayElements(ident, i, 0);
	}
}

JNIEXPORT void JNICALL
Java_javametrics_regListener(JNIEnv *env, jclass clazz, jobject obj) {
	api_callback = env->NewGlobalRef(obj);
	registerListener(&sendMsg);
}

JNIEXPORT void JNICALL
Java_javametrics_deregListener(JNIEnv *env, jobject obj) {
	deregisterListener();
}

JNIEXPORT void JNICALL
Java_javametrics_sendMessage(JNIEnv *env, jobject obj, jstring topic, jbyteArray ident) {

	const char *s = env->GetStringUTFChars(topic,NULL);
	if (s) {
		jboolean isCopy;
		jbyte* i = env->GetByteArrayElements(ident, &isCopy);
		sendControl(s, env->GetArrayLength(ident), (void *)i);
		env->ReleaseStringUTFChars(topic,s);
		env->ReleaseByteArrayElements(ident, i, 0);
	}
}

}



