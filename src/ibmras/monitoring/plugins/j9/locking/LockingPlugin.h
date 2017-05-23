/*
 * LockingPlugin.h
 *
 *  Created on: 19 Feb 2016
 *      Author: Admin
 */

#ifndef ibmras_monitoring_plugins_j9_lockingplugin_h
#define ibmras_monitoring_plugins_j9_lockingplugin_h

#include "ibmras/monitoring/AgentExtensions.h"
#include "ibmras/monitoring/Typesdef.h"
#include "ibmras/monitoring/connector/Receiver.h"
#include "ibmras/monitoring/Plugin.h"
#include "healthcenter.h"

namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace locking {

class LockingPlugin : public ibmras::monitoring::connector::Receiver, public ibmras::monitoring::Plugin {
public:
	LockingPlugin(jvmFunctions* jvmF);
	static LockingPlugin* getPlugin(jvmFunctions* jvmF);
	static LockingPlugin* getPlugin();
	static void* getReceiver();
	virtual ~LockingPlugin();

	static pullsource* registerPullSource(agentCoreFunctions aCF, uint32 provID);
	int startReceiver();
	int stopReceiver();
	monitordata* pullInt();
	void pullcompleteInt(monitordata* data);
	void receiveMessage(const std::string &id, uint32 size, void* data);
	jvmFunctions* getjvmF();
	static unsigned int dump_read_u4(char * dp);
	uint64_t gettmstt();
	static int qcmp_jlm(const void * p1, const void * p2);

private:

	static void publishConfig();
	static bool isEnabled();
	static void setState(const std::string &newState);
	//static char* reportLocking(JNIEnv *env);
	//char* monitor_dump_event(JNIEnv *env);

	static void dump_read(void * dp, void * res, int size);

	//static void getCycles(uint64_t * t);
	//static UINT64 read_cycles_on_processor(int cpu);
	JNIEnv* env;
	jvmFunctions* jvmF;
	uint64_t tm_stt;
};

#define JVMPI_MONITOR_JAVA              0x01
#define JVMPI_MONITOR_RAW               0x02
#define PITRACE_ERROR					-1

monitordata* pullWrapper();
void pullCompleteWrapper(monitordata* data);
int startWrapper();
int stopWrapper();
const char* getLockVersion();
char * dupJavaStr(const char * jnm);
void dealloc_report_lines(char *lines[], int count);
void* hc_realloc_ptr_array(char** source[], int currentSize, int newSize);
char* join_strings(char *strings[], int count);
uint64_t readCyclesOnProcessor(int cpu);
uint64_t getPlatformCycles(void);
UINT64 read_cycles_on_processor(int cpu);
void getCycles(uint64_t * t);
char* monitor_dump_event(JNIEnv *env);
char* reportLocking(JNIEnv *env);

struct __jdata
{
    double miss;
    double util;
    double averht;
    int gets;
    int rec;
    int nrec;
    int fast;
    int slow;
    int tier2;
    int tier3;
    char * mnm;
    unsigned int htlo;
    unsigned int hthi;
    double dhtm;
};
typedef struct __jdata jdata_t;

union rdval
{
    struct
    {
#if defined __BIG_ENDIAN
        uint32_t hval;
        uint32_t lval;
#else  /* kernel code, little endian */
#if defined __LITTLE_ENDIAN
        uint32_t lval;
        uint32_t hval;
#else
#error "Undetermined endian!"
#endif
#endif

    } sval;
    uint64_t cval;
};

}
}
}
}
}


#endif /* LOCKINGPLUGIN_H_ */
