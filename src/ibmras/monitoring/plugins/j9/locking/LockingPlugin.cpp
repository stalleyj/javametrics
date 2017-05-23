/*
 * LockingPlugin.cpp
 *
 *  Created on: 19 Feb 2016
 *      Author: Admin
 */

#if defined(_ZOS)
#define _XOPEN_SOURCE_EXTENDED 1
#endif

#include <jni.h>
#include "jvmti.h"
#include <cstdlib>
#include <sstream>
#include <ctime>
#include <stdint.h>
#include <stdio.h>
#include <cstring>
#include <string>
#include <vector>
#include "ibmras/monitoring/plugins/j9/locking/LockingPlugin.h"
#include "ibmras/common/logging.h"
#include "ibmras/common/util/strUtils.h"
#include "ibmras/monitoring/plugins/j9/DumpHandler.h"
#include <iostream>

#include "ibmras/common/MemoryManager.h"
#include "healthcenter.h"

#include "ibmras/monitoring/agent/Agent.h"
#include "ibmras/monitoring/plugins/j9/Util.h"
#include "ibmras/common/port/Process.h"


#if defined(WINDOWS)
#include <windows.h>
#include <Psapi.h>
#include <WinBase.h>
#include <intrin.h>
uint64_t rdtsc()
{
	return __rdtsc();
}
#else /* Unix platforms */
#define _OE_SOCKETS
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#if defined(LINUX)
#include <sys/time.h>
#endif

#if defined(_ZOS)
#include <sys/time.h>
#include <unistd.h>
#endif

#if defined(_AIX)
#include <builtins.h>
#endif

namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace locking {

IBMRAS_DEFINE_LOGGER("lockingplugin")
;

LockingPlugin* instance = NULL;
static uint32 provID;
bool enabled = true;


LockingPlugin::LockingPlugin(jvmFunctions* jvmF) :
				env(NULL), jvmF(jvmF) {
	IBMRAS_DEBUG(debug, ">>>LockingPlugin::LockingPlugin");
	name = "locking";
	pull = registerPullSource;
	push = NULL;
#ifndef ORACLE
	tm_stt = read_cycles_on_processor(0);
	jvmF->setVMLockMonitor(jvmF->pti, 1);
#endif
	start = startWrapper;
	stop = stopWrapper;
	getVersion = getLockVersion;
	type = ibmras::monitoring::plugin::data
			| ibmras::monitoring::plugin::receiver;
	recvfactory = (RECEIVER_FACTORY)LockingPlugin::getReceiver;
	confactory = NULL;
	IBMRAS_DEBUG(debug, "<<<LockingPlugin::LockingPlugin");
}

LockingPlugin::~LockingPlugin() {

}

LockingPlugin* LockingPlugin::getPlugin(jvmFunctions* jvmF) {
	IBMRAS_DEBUG(debug, ">>>LockingPlugin::getPlugin");
	if(!instance) {
		instance = new LockingPlugin(jvmF);
	}
	return instance;
}

LockingPlugin* LockingPlugin::getPlugin() {
	IBMRAS_DEBUG(debug, ">>>LockingPlugin::getPlugin");
	return instance;
}

void* LockingPlugin::getReceiver() {
	IBMRAS_DEBUG(debug, ">>>LockingPlugin::getReceiver");
	if(!instance) {
		IBMRAS_DEBUG(debug, "<<<LockingPlugin::getReceiver[NULL]");
		return NULL;
	}
	IBMRAS_DEBUG(debug, "<<<LockingPlugin::getReceiver[OK]");
	return (Receiver*)instance;
}

pullsource* LockingPlugin::registerPullSource(agentCoreFunctions aCF,uint32 prov) {
	IBMRAS_DEBUG(debug, ">>>LockingPlugin::registerPullSource");
	pullsource* src = new pullsource();

	src->header.name = "locking";
	provID = prov;
	src->header.description = "Locking information";
	src->header.sourceID = 0;
	src->header.capacity = 256 * 1024;
	src->next = NULL;
	src->callback = pullWrapper;
	src->complete = pullCompleteWrapper;
	src->pullInterval = 60;
	IBMRAS_DEBUG(debug, "<<<LockingPlugin::registerPullSource");
	return src;
}

/***************************************************************************************
 * Receiver-related methods:
 * *************************************************************************************/

void LockingPlugin::publishConfig() {
	ibmras::monitoring::agent::Agent* agent =
			ibmras::monitoring::agent::Agent::getInstance();

	ibmras::monitoring::connector::ConnectorManager *conMan =
			agent->getConnectionManager();

	std::string msg = "locking_subsystem=";
	if (isEnabled()) {
		msg += "on";
	} else {
		msg += "off";
	}

	conMan->sendMessage("configuration/locking", msg.length(),
			(void*) msg.c_str());
}

void LockingPlugin::receiveMessage(const std::string &id, uint32 size, void* data) {
	std::string message((const char*) data, size);
	if (id == "locking") {
		std::size_t found = message.find(',');
		std::string command = message.substr(0, found);
		std::string rest = message.substr(found + 1);

		setState(command);
	}
}

int LockingPlugin::startReceiver() {
	publishConfig();
	return 0;
}

int LockingPlugin::stopReceiver() {
	return 0;
}

bool LockingPlugin::isEnabled() {
	return enabled;
}

void LockingPlugin::setState(const std::string &newState) {
	enabled = ibmras::common::util::equalsIgnoreCase(newState, "on");
	if (instance) {
		instance->publishConfig();
	}
}

/***************************************************************************************
 * Data-production/data-transfer related methods:
 * *************************************************************************************/

monitordata* LockingPlugin::pullInt() {
	monitordata* data = new monitordata;

	IBMRAS_DEBUG(debug, ">>>LockingPlugin::pullInt");

	ibmras::monitoring::plugins::j9::setEnv(&env, "Health Center (locking)", jvmF->theVM, true);

	if(!env) {
		IBMRAS_DEBUG(warning, "The JNI env was not set up");
		IBMRAS_DEBUG(debug, "<<<LockingPlugin::pullInt[JNIenv=NULL]");
		return NULL;
	}

	data->size = 0;
	data->data = NULL;

	if (isEnabled()) {
		data->persistent = false;
		std::stringstream ss;

		data->provID = provID;
		data->sourceID = 0;

		unsigned long long millisecondsSinceEpoch;

#if defined(WINDOWS)

		SYSTEMTIME st;
		GetSystemTime(&st);

		millisecondsSinceEpoch = time(NULL)*1000+st.wMilliseconds;

#else
		struct timeval tv;
		gettimeofday(&tv, NULL);

		millisecondsSinceEpoch = (unsigned long long) (tv.tv_sec) * 1000
				+ (unsigned long long) (tv.tv_usec) / 1000;
#endif

		ss << "reportTime," << millisecondsSinceEpoch << ",";

		char* value = reportLocking(env);

		ss << value;

		ibmras::monitoring::plugins::j9::hc_dealloc(reinterpret_cast<unsigned char**>(&value));

		std::string lockingdata = ss.str();

		int len = lockingdata.length();

		char* sval = ibmras::common::util::createAsciiString(lockingdata.c_str());

		if (sval) {
			data->size = len;
			data->data = sval;
		}
	}
	IBMRAS_DEBUG(debug, "<<<LockingPlugin::pullInt");
	return data;
}

void LockingPlugin::pullcompleteInt(monitordata* data) {
	IBMRAS_DEBUG(debug, ">>>LockingPlugin::pullcompleteInt");
	if (data) {
			hc_dealloc((unsigned char**) (&(data->data)));
			delete data;
			IBMRAS_DEBUG(debug, "<<<LockingPlugin::pullcompleteInt[data deleted]");
	} else {
		if (env) {
			IBMRAS_DEBUG_1(debug, "Detaching thread %s", name.c_str());
			jvmF->theVM->DetachCurrentThread();
			env = NULL;
			IBMRAS_DEBUG(debug, "<<<LockingPlugin::pullcompleteInt[thread detached]");
		}
	}
}

jvmFunctions* LockingPlugin::getjvmF() {
	return instance->jvmF;
}
uint64_t LockingPlugin::gettmstt() {
	return instance->tm_stt;
}

char* reportLocking(JNIEnv *env) {
	char * reportData = NULL;

	IBMRAS_DEBUG(debug, "> reportLocking");

	/* Create the report to pass to the gui */
	reportData = monitor_dump_event(env);

	IBMRAS_DEBUG(debug, "< reportLocking");
	return reportData;
}

char* monitor_dump_event(JNIEnv *env) {
	void ** q = NULL;
	void * p = 0;
	LockingPlugin* lp = LockingPlugin::getPlugin();

	UINT64 tm_delta;
	UINT64 tm_curr;

	static int dcnt = 0;

	char * beg;
	char * end;
	char * stt;
	unsigned char ty;
	int psz = sizeof(void *);

	int reportArraySize = 100;
	char ** reportLineArray = NULL;

	char * finalReport = NULL; /* This will contain the full report to be passed back */
	int reportLineCount = 0; /* A count of the number of lines in the report */

	int rawcnt = 0;
	int infcnt = 0;

	char buffer[1000];

	const char* repjlmfmt =
			"%s, %5.0f, %8d, %8d, %8d, %8d, %8d, %8d, %5.0f, %8.0f,  %s,";

	const char* javaMon = "javaMonitor";
	const char* systemMon = "systemMonitor";
	const char* reportEnd = "reportcomplete";

	jdata_t * jp = NULL;

	int JMONMAX;
	int SMONMAX;

	int arrj_size;
	int arrs_size;

	jdata_t ** arrjmon = NULL;
	jdata_t ** arrsmon = NULL;

	jvmtiEnv* pti = lp->getjvmF()->pti;
	/*** code starts ***/
	IBMRAS_DEBUG(debug, "> monitor_dump_event");


	// if we are unable to get any timing info, return a null report
	// This should only affect ARM builds for now
	if(read_cycles_on_processor(0) == 0) {
        IBMRAS_DEBUG(fine, "No timing info");
		goto cleanup;;
	}


	/* before we do anything else, create this - we need to be sure we can free it later. */
	finalReport = (char*) ibmras::monitoring::plugins::j9::hc_alloc(
			sizeof(char) * 500);
	if (finalReport == NULL) {
        IBMRAS_DEBUG(fine, "Unable to allocate final report");
		goto cleanup;
	}

	reportLineArray = (char**) ibmras::monitoring::plugins::j9::hc_alloc(
			reportArraySize * sizeof reportLineArray);
	if (reportLineArray == NULL) {
        IBMRAS_DEBUG(fine, "Unable to allocate report line array");
		goto cleanup;
	}

	lp->getjvmF()->dumpVMLockMonitor(pti, &p);
	if (0 == p) {
		/* if the dumpVMlockMonitor call has failed returned a blank string
		 * this seems to happen quite often (intermittently 50% of the time?)
		 * there doesn't seem to be an impact on our code
		 */
        IBMRAS_DEBUG(fine, "dumpVMLockMonitor failed");
		goto cleanup;
	} else {
		q = (void **) p;
	}

	//int pid = ibmras::common::port::getProcessId();

	dcnt++; /* Get current report run number */

	beg = reinterpret_cast<char*>(q[0]);
	end = reinterpret_cast<char*>(q[1]);

	/* calc max entries divided by 32*/
	JMONMAX = (int) ((end - beg) / 32);
	SMONMAX = (int) (end - beg);

	arrj_size = JMONMAX * sizeof(jdata_t *);
	arrjmon = (jdata_t **) ibmras::monitoring::plugins::j9::hc_alloc(
			arrj_size);
	if (arrjmon == NULL) {
        IBMRAS_DEBUG(fine, "Unable to allocate Java Monitor array");
		goto cleanup;
	}

	arrs_size = SMONMAX * sizeof(jdata_t *);
	arrsmon = (jdata_t **) ibmras::monitoring::plugins::j9::hc_alloc(
			arrs_size);
	if (arrsmon == NULL) {
        IBMRAS_DEBUG(fine, "Unable to allocate System Monitor array");
		goto cleanup;
	}

	tm_curr = read_cycles_on_processor(0);
	tm_delta = tm_curr - lp->gettmstt();

	/* output interval time */
	sprintf(buffer, "IntervalTime, %" _P64 "d,", tm_delta);
	reportLineArray[reportLineCount] =
			reinterpret_cast<char*>(ibmras::monitoring::plugins::j9::hc_alloc(
					strlen(buffer) + 1));
	if (reportLineArray[reportLineCount] == NULL) {
        IBMRAS_DEBUG(fine, "Unable to allocate report line string");
		goto cleanup;
	}
	strcpy(reportLineArray[reportLineCount++], buffer);
	IBMRAS_DEBUG_2(fine, "Report line size %d, contains %s", strlen(reportLineArray[reportLineCount-1]), reportLineArray[reportLineCount-1]);

	stt = beg;

	while (stt < end) {
		//unsigned char held;
		unsigned int htlo, hthi;
		double dhtm;

		jp = (jdata_t *) ibmras::monitoring::plugins::j9::hc_alloc(
				sizeof(jdata_t));
		if (jp == NULL) {
            IBMRAS_DEBUG(fine, "Unable to allocate Java Monitor");
			goto cleanup;
		}

		ty = (unsigned char) *stt;
		stt++;
		//held = (unsigned char) *stt;
		stt++;

		/* The VM still uses JVMPI constants (1 and 2) even though it's not JVMPI anymore
		 We replicate these in our header file. */
		if (ty == JVMPI_MONITOR_JAVA) {
			arrjmon[infcnt] = jp;
			infcnt++;
			if (infcnt >= JMONMAX) {
                IBMRAS_DEBUG_1(debug, "Stopping JLM, Java Monitor Count %d\n",
						JMONMAX);
				sprintf(finalReport, "Stopping JLM, Java Monitor Count %d\n",
						JMONMAX);
				goto returnReport;
			}
		} else if (ty == JVMPI_MONITOR_RAW) {
			arrsmon[rawcnt] = jp;
			rawcnt++;
			if (rawcnt >= SMONMAX) {
                IBMRAS_DEBUG_1(debug, "Stopping JLM, Raw Monitor Count %d\n",
						SMONMAX);
				sprintf(finalReport, "Stopping JLM, Raw Monitor Count %d\n",
						SMONMAX);
				goto returnReport;
			}
		} else {
            IBMRAS_DEBUG(debug, "Stopping JLM, Bad Data\n");
			sprintf(finalReport, "Stopping JLM, Bad Data\n");
			goto returnReport;
		}

		jp->gets = LockingPlugin::dump_read_u4(stt);
		stt += 4;
		jp->slow = LockingPlugin::dump_read_u4(stt);
		stt += 4;
		jp->rec = LockingPlugin::dump_read_u4(stt);
		stt += 4;
		jp->tier2 = LockingPlugin::dump_read_u4(stt);
		stt += 4;
		jp->tier3 = LockingPlugin::dump_read_u4(stt);
		stt += 4;
		hthi = (unsigned int) LockingPlugin::dump_read_u4(stt);
		stt += 4;
		htlo = (unsigned int) LockingPlugin::dump_read_u4(stt);
		stt += 4;

		jp->hthi = hthi;
		jp->htlo = htlo;

		stt += psz;

		jp->mnm = ibmras::monitoring::plugins::j9::locking::dupJavaStr(stt);
		if (jp->mnm == NULL) {
			goto returnReport;
		}

		stt += strlen(jp->mnm) + 1;

		jp->nrec = jp->gets - jp->rec;
		jp->fast = jp->nrec - jp->slow;

		if (jp->gets > 0) {
			dhtm = (double) htlo;
			dhtm += 1024.0 * 1024.0 * 4096.0 * (double) hthi;
			jp->dhtm = dhtm;
			if (tm_delta == 1){
				jp->util = 1;
			}
			else {
				jp->util = 100.0 * dhtm / (double) (INT64) tm_delta;
			}

			if (jp->nrec > 0) {
				jp->averht = dhtm / (double) (INT64) jp->nrec;
				jp->miss = 100.0 * jp->slow / (double) jp->nrec;
			} else {
				jp->averht = 0.0;
				jp->miss = 0.0;
			}
		} else {
			jp->util = jp->miss = jp->averht = 0.0;
		}
	}

	qsort((void *) arrsmon, rawcnt, sizeof(jdata_t *), LockingPlugin::qcmp_jlm);

	/*  This is the start of the block that outputs the system monitors to the report.  The calculations have all been
	 done at this point and stored in the arrsmon[] array */

	for (int i = 0; i < rawcnt; i++) {
		jp = arrsmon[i];

		/* Add the reportLine to the array */
		sprintf(buffer, repjlmfmt, systemMon, jp->miss, jp->gets, jp->nrec,
				jp->slow, jp->rec, jp->tier2, jp->tier3, jp->util, jp->averht,
				jp->mnm);
		reportLineArray[reportLineCount] =
				reinterpret_cast<char*>(ibmras::monitoring::plugins::j9::hc_alloc(
						strlen(buffer) + 1));
		if (reportLineArray[reportLineCount] == NULL) {
			goto cleanup;
		}
		strcpy(reportLineArray[reportLineCount++], buffer);
		IBMRAS_DEBUG_2(fine, "%d, %s", strlen(reportLineArray[reportLineCount-1]), reportLineArray[reportLineCount-1]);

		/*
		 * check to make sure we have not grown to big for the array
		 */
		if (reportLineCount >= reportArraySize) {
			/* add another 100 lines to the report */
			reportArraySize += 100;

			reportLineArray =
					(char**) ibmras::monitoring::plugins::j9::locking::hc_realloc_ptr_array(
							&reportLineArray, reportLineCount, reportArraySize);
			if (reportLineArray == NULL) {
				goto cleanup;
			}
		}
	}
	/*  This is the end of the system monitor block */

	qsort((void *) arrjmon, infcnt, sizeof(jdata_t *), LockingPlugin::qcmp_jlm);

	/*  This is the start of the block that outputs the java program monitors to the report.  The calculations have all been
	 done at this point and stored in the arrjmon[] array */
	for (int i = 0; i < infcnt; i++) {
		jp = arrjmon[i];

		/* Add the reportLine to the array */
		sprintf(buffer, repjlmfmt, javaMon, jp->miss, jp->gets, jp->nrec,
				jp->slow, jp->rec, jp->tier2, jp->tier3, jp->util, jp->averht,
				jp->mnm);
		reportLineArray[reportLineCount] =
				reinterpret_cast<char*>(ibmras::monitoring::plugins::j9::hc_alloc(
						strlen(buffer) + 1));
		if (reportLineArray[reportLineCount] == NULL) {
			goto cleanup;
		}
		strcpy(reportLineArray[reportLineCount++], buffer);
		IBMRAS_DEBUG_2(fine, "%d, %s", strlen(reportLineArray[reportLineCount-1]), reportLineArray[reportLineCount-1]);

		/*
		 * check to make sure we have not grown to big for the array
		 */
		if (reportLineCount >= reportArraySize) {
			/* add another 100 lines to the report */
			reportArraySize += 100;

			reportLineArray =
					(char**) ibmras::monitoring::plugins::j9::locking::hc_realloc_ptr_array(
							&reportLineArray, reportLineCount, reportArraySize);
			if (reportLineArray == NULL) {
				goto cleanup;
			}
		}
	}

	/*  This is the end of the java monitor block */
	reportLineArray[reportLineCount] =
			reinterpret_cast<char*>(ibmras::monitoring::plugins::j9::hc_alloc(
					strlen(reportEnd) + 1));
	if (reportLineArray[reportLineCount] == NULL) {
		goto cleanup;
	}
	sprintf(reportLineArray[reportLineCount++], "%s", reportEnd);
	IBMRAS_DEBUG_1(fine, "%s", reportLineArray[reportLineCount-1]);

	/* if we get this far we've not used the original finalReport that we malloced memory for, join_strings will
	 * malloc more memory so lets free the original one
	 */
	ibmras::monitoring::plugins::j9::hc_dealloc(
			(unsigned char**) &finalReport);

	/* Now that the array is complete we need to merge all those strings into one final one to pass
	 pass back */
	finalReport = ibmras::monitoring::plugins::j9::locking::join_strings(
			reportLineArray, reportLineCount);
	if (finalReport != NULL) {
		goto returnReport;
	}

	/* cleanup - returns a NULL report */
	cleanup: IBMRAS_DEBUG(debug, "monitor_dump_event: cleanup");
	ibmras::monitoring::plugins::j9::hc_dealloc(
			(unsigned char**) &finalReport);
	/* returnReport - returns a report to be deallocated by the caller */

	returnReport:

	IBMRAS_DEBUG(debug, "monitor_dump_event: returnReport");

	if (p) {
		ibmras::monitoring::plugins::j9::hc_dealloc((unsigned char**) &q[0]);
		ibmras::monitoring::plugins::j9::hc_dealloc((unsigned char**) &p);
	}

	for (int i = 0; i < rawcnt; i++) {
		ibmras::monitoring::plugins::j9::hc_dealloc(
				(unsigned char**) &arrsmon[i]->mnm);
		ibmras::monitoring::plugins::j9::hc_dealloc(
				(unsigned char**) &arrsmon[i]);
	}

	for (int i = 0; i < infcnt; i++) {
		ibmras::monitoring::plugins::j9::hc_dealloc(
				(unsigned char**) &arrjmon[i]->mnm);
		ibmras::monitoring::plugins::j9::hc_dealloc(
				(unsigned char**) &arrjmon[i]);
	}

	ibmras::monitoring::plugins::j9::hc_dealloc((unsigned char**) &arrjmon);
	ibmras::monitoring::plugins::j9::hc_dealloc((unsigned char**) &arrsmon);

	ibmras::monitoring::plugins::j9::locking::dealloc_report_lines(reportLineArray,
			reportArraySize);
	ibmras::monitoring::plugins::j9::hc_dealloc(
			(unsigned char**) &reportLineArray);

	IBMRAS_DEBUG(debug, "< monitor_dump_event");
	return finalReport;
}

UINT64 read_cycles_on_processor(int cpu) {
	UINT64 cycles = 0;

#ifdef _ZOS
	__stck( (unsigned long long *)&cycles); /* Use unaltered time for JLM */
#else
#if defined (_LINUX) && !defined(_PPC) && !defined(_S390)
	cycles = readCyclesOnProcessor(cpu);
#else
	getCycles(&cycles);
#endif
#endif

	return cycles;
}


/*
 _GetCycles()
 **************
 This is an architecture specific routine.
 */
uint64_t getPlatformCycles(void) {
	union rdval time;
#ifdef CONFIG_IA64
	int result = 0;
#endif

#if defined(_WINDOWS)
	return rdtsc();
#endif

#if defined (_SOLARIS)
	int result = 0;
#endif

#if defined (_HPIA)
	int result = 0;
#endif

#if defined (_HPRISC)
	int result = 0;
#endif

#if defined(_LINUX) && !defined(_PPC) && !defined(_S390)
#if !defined(_ARM)
	uint32_t lo, hi;
	__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
	return (uint64_t) hi << 32 | lo;
#else
	uint64_t result = 0;
	return result;
//	long long result = 0;
//	if (read(cpuCycleHandle, &result, sizeof(result)) < sizeof(result)) return 0;
//	return (uint64_t)result;
#endif // !defined(_ARM)

#endif

#if defined(CONFIG_S390) || defined(CONFIG_S390X) || defined(_S390)
	__asm__ __volatile__("la     1,%0\n stck    0(1)":"=m"(time.cval)
			::"cc", "1");
#endif


#if defined(_AIX)
#if defined(_PPC) || defined(CONFIG_PPC64)
	IBMRAS_DEBUG(debug, "@Locking, line 680");
    union rdval temp1;
    do
	{
        __fence();
#if defined(_64BIT)
        temp1.cval = __mftb();
        time.cval = __mftb();
#else
        temp1.sval.hval = __mftbu();
		time.sval.lval = __mftb();
        time.sval.hval = __mftbu();
#endif
        __fence();
        IBMRAS_DEBUG_2(debug, "temp1.sval.hval = %i, time.sval.hval=%i", temp1.sval.hval, time.sval.hval);
	} while (temp1.sval.hval != time.sval.hval);

#endif
#else
#if defined(_PPC) || defined(CONFIG_PPC64)
	IBMRAS_DEBUG(debug, "@Locking, line 680");
	uint32_t temp1 = 1;

	time.sval.hval = 2;
	while (temp1 != time.sval.hval)
	{
		__asm__ __volatile__("mftbu %0":"=r"(temp1));
		__asm__ __volatile__("mftb  %0":"=r"(time.sval.lval));
		__asm__ __volatile__("mftbu %0":"=r"(time.sval.hval));
	}
#endif
#endif


#ifdef CONFIG_IA64
	__asm__ __volatile__("mov %0=ar.itc":"=r"(time.cval)::"memory");
#ifdef CONFIG_ITANIUM
	while (__builtin_expect((__s32) result == -1, 0))
	__asm__ __volatile__("mov %0=ar.itc":"=r"(time.cval)::"memory");
#endif
#endif
	return time.cval;
}

void getCycles(uint64_t * t) {
	*t = getPlatformCycles();
}

#if defined (_LINUX) && !defined(_PPC) && !defined(_S390)
/*
 ReadCyclesOnProcessor()
 ************************
 Read cycles on a specified processor
 */

uint64_t readCyclesOnProcessor(int cpu) {
	cpu_set_t prev_mask; /* current processor affinity mask */
	cpu_set_t new_mask; /* mask to attach to a processor */
	uint64_t mycycles;

	/* get affinity mask for the current process */
	/* the call for this depends on which version of glibc we are using */
//#ifdef _NEW_GLIBC
	int sched_get_return_code = sched_getaffinity(0, sizeof(cpu_set_t),
			&prev_mask);
//#else
//    int sched_get_return_code = sched_getaffinity(0, &prev_mask);
//#endif

	if (sched_get_return_code != 0) {
		printf("sched_getaffinity failed\n");
		return PITRACE_ERROR;
	}
	/* switch & read cycles      */

	CPU_ZERO(&new_mask);
	CPU_SET(cpu, &new_mask);

//#ifdef _NEW_GLIBC
	int sched_set_return_code = sched_setaffinity(0, sizeof(cpu_set_t),
			&new_mask);
//#else
//    int sched_set_return_code = sched_setaffinity(0, &new_mask);
//#endif

	if (sched_set_return_code != 0) {
		printf("sched_setaffinity failed\n");
		return PITRACE_ERROR;
	}

	ibmras::common::port::sleep(0);

	mycycles = getPlatformCycles();

	/* restore previous mask */

//#ifdef _NEW_GLIBC
	int sched_reset_return_code = sched_setaffinity(0, sizeof(cpu_set_t),
			&prev_mask);
//#else
//    int sched_reset_return_code = sched_setaffinity(0, &prev_mask);
//#endif

	if (sched_reset_return_code != 0) {
		printf("sched_setaffinity failed\n");
		return PITRACE_ERROR;
	}

	ibmras::common::port::sleep(0);

	return mycycles;
}
#endif

unsigned int LockingPlugin::dump_read_u4(char * dp) {
	unsigned int temp;

	dump_read((void *) dp, (void *) &temp, 4);
	temp = ntohl(temp);
	return temp;
}

void LockingPlugin::dump_read(void * dp, void * res, int size) {
	memcpy(res, dp, size);
	return;
}

int LockingPlugin::qcmp_jlm(const void * p1, const void * p2) {
	jdata_t * r;
	jdata_t * s;

	r = *(jdata_t **) p1;
	s = *(jdata_t **) p2;

	if (r->gets > s->gets) {
		return -1;
	}
	if (r->gets < s->gets) {
		return 1;
	}

	return 0;
}

/***************************************************************************************
 * Native string manipulation related methods:
 * *************************************************************************************/

char * dupJavaStr(const char * jnm) {
	char* cp = NULL;
	if (NULL != jnm) {
		cp = (char*) ibmras::monitoring::plugins::j9::hc_alloc(
				strlen(jnm) + 1);
		if (NULL == cp) {
			return NULL;
		} else {
			/* jnm is valid, so is cp */
			strcpy(cp, jnm);
			ibmras::common::util::force2Native(cp);
		}
	} else {
		/* malloc enough space for the word null */
		cp = (char*) ibmras::monitoring::plugins::j9::hc_alloc(
				sizeof(char) * 4 + 1);
		if (NULL == cp) {
			return NULL;
		}
		strcpy(cp, "NULL");
	}
	return cp;
}




/************
 * Function to join an array of strings                            *
 * this function allocates memory that must be freed by the caller *
 ************/
char* join_strings(char *strings[], int count) {
	char* str = NULL; /* Pointer to the joined strings  */
	size_t total_length = 0; /* Total length of joined strings */
	int i = 0; /* Loop counter                   */

	/* Find total length of joined strings */
	for (i = 0; i < count; i++) {
		if (strings[i] != NULL) {
			total_length += strlen(strings[i]);
		}
	}
	++total_length; /* For joined string terminator */

	str = (char*) ibmras::monitoring::plugins::j9::hc_alloc(total_length); /* Allocate memory for joined strings */
	if (NULL == str) {
		return NULL;
	}
	str[0] = '\0'; /* Empty string we can append to      */

	/* Append all the strings */
	for (i = 0; i < count; i++) {
		if (strings[i] != NULL) {
			strcat(str, strings[i]);
		}
	}

	return str;
}

/***************************************************************************************
 * Memory allocation related methods:
 * *************************************************************************************/

void dealloc_report_lines(char *lines[], int count) {
	int i;
	//IBMRAS_DEBUG(debug,  "> dealloc_report_lines");

	if (lines != NULL) {
		/* Need to free the memory for our array */
		for (i = 0; i < count; i++) {
			ibmras::monitoring::plugins::j9::hc_dealloc((unsigned char**) &lines[i]);
		}
	}
	//IBMRAS_DEBUG(debug,  "< dealloc_report_lines");
}

void* hc_realloc_ptr_array(char** source[], int currentSize, int newSize) {
	char** buffer = NULL;
	int i;

	if (currentSize >= newSize) {
		return *source;
	}

	buffer = (char**) ibmras::monitoring::plugins::j9::hc_alloc(newSize * sizeof(char*));
	if (buffer != NULL) {
		for (i = 0; i < currentSize; i++) {
			buffer[i] = (*source)[i];
		}
	}
	ibmras::monitoring::plugins::j9::hc_dealloc((unsigned char**) source);
	*source = buffer;
	return buffer;
}

/*-------------------------------------------------------------------------------------
 * These are the namespace functions that are used to avoid the restrictions imposed
 * by the defined typedefs for callback functions. Non-static member function pointers
 * have a different prototype than the one generically typedef'd in the headers.
 *-----------------------------------------------------------------------------------*/

monitordata* pullWrapper() {
		return instance->pullInt();
}

void pullCompleteWrapper(monitordata* data) {
	instance->pullcompleteInt(data);
}

int startWrapper() {
	return instance->startReceiver();
}

int stopWrapper() {
	return instance->stopReceiver();
}

const char* getLockVersion() {
	return "1.0";
}


}//locking
}//j9
}//plugins
}//monitoring
}//ibmras
