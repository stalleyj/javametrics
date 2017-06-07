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
#include "javametrics.h"
#include "ibmras/common/Properties.h"
#include "ibmras/common/util/strUtils.h"
#include "ibmras/common/port/Process.h"
#include "ibmras/vm/java/JVMTIMemoryManager.h"

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
void (*apiPushData)(const char*);

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
	std::cerr << "in agent onload";
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
	std::string apiPlugin = fileJoin(pluginPath, "apiplugin.dll");
	HMODULE handle = LoadLibrary(apiPlugin.c_str());
	if (handle == NULL) {
		std::cerr << "API Connector Listener: failed to open apiplugin.dll \n";
		return NULL;
	}
	FARPROC apiFunc = GetProcAddress(handle, const_cast<char *>(funcName.c_str()));
	if (apiFunc == NULL) {
		std::cerr << "API Connector Listener: cannot find symbol '" << funcName << " in apiplugin.dll \n";
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
			"com.ibm.javametrics.path");
	if (agentRemotePath.length() != 0) {
		std::size_t libPos = agentLibPath.find("/lib");
		std::string relativeLibPath = agentLibPath.substr(libPos);
		agentLibPath = agentRemotePath + relativeLibPath;
	}

	agent->addPlugin(agentLibPath, "apiplugin");

	registerListener =
			(void (*)(
					void (*func)(const char*, unsigned int,
							void*))) getApiFunc(agentLibPath, std::string("registerListener"));
    deregisterListener = (void (*)())getApiFunc(agentLibPath, std::string("deregisterListener"));
    sendControl	= (void (*)(const char*, unsigned int,
			void*)) getApiFunc(agentLibPath, std::string("sendControl"));
    apiPushData = (void (*)(const char*)) getApiFunc(agentLibPath, std::string("apiPushData"));
    
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

// 	agent->addPlugin(
// 			ibmras::monitoring::plugins::j9::api::AppPlugin::getInstance(tDPP));
}

void initialiseProperties(const std::string &options) {
	agent = ibmras::monitoring::agent::Agent::getInstance();
	agent->setAgentProperty("launch.options", options);
	agent->setLogLevels();
}
/**
 * launch agent code
 */
int launchAgent() {

	agent = ibmras::monitoring::agent::Agent::getInstance();

	agent->setLogLevels();

// now we have the system properties, AIX and z/OS can load the MQTT and API plugins.
#if defined(_AIX) || defined(_ZOS)
	addAPIPlugin();
#endif

	std::string agentVersion = agent->getVersion();
	IBMRAS_LOG_1(fine, "javametrics Agent %s", agentVersion.c_str());
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
// 		rc = ibmras::monitoring::plugins::j9::setEnv(&ourEnv,
// 				"Application metrics for Java (javametrics)", theVM, false);
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
Java_com_ibm_javametrics_JavametricsAgentConnector_regListener(JNIEnv *env, jclass clazz, jobject obj) {
	api_callback = env->NewGlobalRef(obj);
	registerListener(&sendMsg);
}

JNIEXPORT void JNICALL
Java_com_ibm_javametrics_JavametricsAgentConnector_deregListener(JNIEnv *env, jobject obj) {
	deregisterListener();
}

JNIEXPORT void JNICALL
Java_com_ibm_javametrics_JavametricsAgentConnector_sendMessage(JNIEnv *env, jobject obj, jstring topic, jbyteArray ident) {

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
Java_com_ibm_javametrics_JavametricsAgentConnector_pushDataToAgent(JNIEnv *env, jobject obj, jstring data) {

	const char *sendData = env->GetStringUTFChars(data,NULL);
	if (sendData) {
		apiPushData(sendData);
		env->ReleaseStringUTFChars(data,sendData);
	}
}

}


