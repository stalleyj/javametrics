/*
 * MemoryPlugin.cpp
 *
 *  Created on: 19 Feb 2016
 *      Author: Admin
 */

#include <cstring>
#include <string>
#include <sstream>
#include "jni.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stddef.h>
#include <sys/types.h>
#include <fcntl.h>
#include <ctime>
#include <iostream>
#include "ibmras/monitoring/plugins/j9/memory/MemoryPlugin.h"
#include "ibmras/monitoring/agent/Agent.h"
#include "healthcenter.h"
#include "ibmras/common/util/memUtils.h"
#include "ibmras/common/util/strUtils.h"
#include "ibmras/common/logging.h"
#include "ibmras/common/MemoryManager.h"
#include "ibmras/monitoring/plugins/j9/Util.h"

#if defined(WINDOWS)
#include <windows.h>
#include <Psapi.h>
#else /* Unix platforms */
#define _OE_SOCKETS
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

typedef size_t UDATA;
typedef long IDATA;

/*########################################################################################################################*/
/*########################################################################################################################*/
/*########################################################################################################################*/

typedef size_t UDATA;
typedef long IDATA;
#if defined(WINDOWS)
#include <windows.h>
#include <winbase.h>
#include <psapi.h>
#pragma comment(lib,"psapi.lib")
#endif /* defined(WIN32) || defined(WIN64) */

#if defined(LINUX)
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/prctl.h>
#endif

#if defined(AIXPPC)
#include <unistd.h>
#include <alloca.h>
#include <procinfo.h>
#include <sys/vminfo.h>
#include <sys/procfs.h>
#include <sys/resource.h>
#include <sys/types.h>
#if !defined(VMINFO_GETPSIZES)

#define VMINFO_GETPSIZES  102 /* report a system's supported page sizes */
#define VMINFO_PSIZE      103 /* report statistics for a page size */

struct vminfo_psize
{
	psize_t psize; /* IN: page size                        */

	/* The rest of this struct is output from vmgetinfo()           */

	uint64_t attr; /* bitmap of page size's attributes     */

	/* Page size attributes reported in the vminfo_psize.attr field: */
#define VM_PSIZE_ATTR_PAGEABLE       0x1  /* page size supports paging  */

	uint64_t pgexct; /* count of page faults                 */
	uint64_t pgrclm; /* count of page reclaims               */
	uint64_t lockexct; /* count of lockmisses                  */
	uint64_t backtrks; /* count of backtracks                  */
	uint64_t pageins; /* count of pages paged in              */
	uint64_t pageouts; /* count of pages paged out             */
	uint64_t pgspgins; /* count of page ins from paging space  */
	uint64_t pgspgouts; /* count of page outs from paging space */
	uint64_t numsios; /* count of start I/Os                  */
	uint64_t numiodone; /* count of iodones                     */
	uint64_t zerofills; /* count of zero filled pages           */
	uint64_t exfills; /* count of exec filled pages           */
	uint64_t scans; /* count of page scans by clock         */
	uint64_t cycles; /* count of clock hand cycles           */
	uint64_t pgsteals; /* count of page steals                 */
	uint64_t freewts; /* count of free frame waits            */
	uint64_t extendwts; /* count of extend XPT waits            */
	uint64_t pendiowts; /* count of pending I/O waits           */

	/*
	 * the next fields need to be computed by vmgetinfo
	 * system call, else their value will be inaccurate.
	 */
	rpn64_t numframes; /* # of real memory frames of this psize */
	rpn64_t numfrb; /* number of pages on free list */
	rpn64_t numclient; /* number of client frames */
	rpn64_t numcompress; /* no of frames in compressed segments */
	rpn64_t numperm; /* number frames non-working segments */
	rpn64_t numvpages; /* accessed virtual pages */
	rpn64_t minfree; /* minimun pages free list (fblru) */
	rpn64_t maxfree; /* maxfree pages free list (fblru) */
#ifndef RPTYPES
#define RPTYPES         2
#endif
	rpn64_t rpgcnt[RPTYPES];/* repaging cnt */
	rpn64_t numpout; /* number of fblru page-outs        */

	rpn64_t numremote; /* number of fblru remote page-outs */
	rpn64_t numwseguse; /* count of pages in use for working seg */
	rpn64_t numpseguse; /* count of pages in use for persistent seg */
	rpn64_t numclseguse; /* count of pages in use for client seg */
	rpn64_t numwsegpin; /* count of pages pinned for working seg */
	rpn64_t numpsegpin; /* count of pages pinned for persistent seg */
	rpn64_t numclsegpin; /* count of pages pinned for client seg */
	rpn64_t numpgsp_pgs; /* # of wseg pages with allocated paging space */

	rpn64_t numralloc; /* number of remote allocations */
	rpn64_t pfrsvdblks; /* number of system reserved blocks */
	rpn64_t pfavail; /* number of pages available for pinning */
	rpn64_t pfpinavail; /* app-level num pages avail for pinning */
	rpn64_t numpermio; /* number of fblru non-w.s. pageouts    */

	rpn64_t system_pgs; /* pages on SCBs marked V_SYSTEM        */
	rpn64_t nonsys_pgs; /* pages on SCBs not marked V_SYSTEM    */
};

#endif /* !defined(VMINFO_GETPSIZES) */
#endif

namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace memory {

IBMRAS_DEFINE_LOGGER("memoryplugin");

MemoryPlugin* instance = NULL;
uint32 provID;
bool enabled;
bool available = true;

MemoryPlugin::MemoryPlugin(jvmFunctions* jvmF) :
				env(NULL), jvmF(jvmF) {
	IBMRAS_DEBUG(debug, ">>>MemoryPlugin::MemoryPlugin");
	name = "memory";
	pull = registerPullSource;
	push = NULL;
	start = startWrapper;
	stop = stopWrapper;
	getVersion = getMemVersion;
	type = ibmras::monitoring::plugin::data
			| ibmras::monitoring::plugin::receiver;
	recvfactory = (RECEIVER_FACTORY)MemoryPlugin::getReceiver;
	confactory = NULL;
	IBMRAS_DEBUG(debug, "<<<MemoryPlugin::MemoryPlugin");

	ibmras::monitoring::agent::Agent* agent =
			ibmras::monitoring::agent::Agent::getInstance();
	std::string enableProp = agent->getAgentProperty("data.memory");
    if (enableProp == "on" || enableProp == "") {
        enabled = true;
    } else {
        enabled = false;
    }
}

MemoryPlugin::~MemoryPlugin() {
}

MemoryPlugin* MemoryPlugin::getPlugin(jvmFunctions* jvmF) {
	IBMRAS_DEBUG(debug, ">>>MemoryPlugin::getPlugin");
	if(!instance) {
		instance = new MemoryPlugin(jvmF);
	}
	return instance;
}

void* MemoryPlugin::getReceiver() {
	IBMRAS_DEBUG(debug, ">>>MemoryPlugin::getReceiver");
	if(!instance) {
		IBMRAS_DEBUG(debug, "<<<MemoryPlugin::getReceiver[NULL]");
		return NULL;
	}
	IBMRAS_DEBUG(debug, "<<<MemoryPlugin::getReceiver[OK]");
	return (Receiver*)instance;
}

pullsource* MemoryPlugin::registerPullSource(agentCoreFunctions aCF,uint32 prov) {
	IBMRAS_DEBUG(debug, ">>>MemoryPlugin::registerPullSource");
	pullsource* src = new pullsource();

	src->header.name = "memory";
	provID = prov;
	src->header.description = "Memory information";
	src->header.sourceID = 0;
	src->header.capacity = 8 * 1024;
	src->next = NULL;
	src->callback = pullWrapper;
	src->complete = pullCompleteWrapper;
	src->pullInterval = 5;
	IBMRAS_DEBUG(debug, "<<<MemoryPlugin::registerPullSource");
	return src;
}

/***************************************************************************************
 * Receiver-related methods:
 * *************************************************************************************/

void MemoryPlugin::publishConfig() {
	ibmras::monitoring::agent::Agent* agent =
			ibmras::monitoring::agent::Agent::getInstance();

	ibmras::monitoring::connector::ConnectorManager *conMan =
			agent->getConnectionManager();

	std::string msg = "memory_subsystem=";
	if (isEnabled()) {
		msg += "on";
	} else {
		msg += "off";
	}

	conMan->sendMessage("configuration/memory", msg.length(),
			(void*) msg.c_str());
}

void MemoryPlugin::receiveMessage(const std::string &id, uint32 size, void* data) {
	IBMRAS_DEBUG(debug, ">>>MemoryPlugin::receiveMessage");
	std::string message((const char*) data, size);

	if (id == "memory") {
		std::size_t found = message.find(',');
		std::string command = message.substr(0, found);
		std::string rest = message.substr(found + 1);

		ibmras::monitoring::plugins::j9::memory::MemoryPlugin::setState(
				command);
	}
	IBMRAS_DEBUG(debug, "<<<MemoryPlugin::receiveMessage");
}

void MemoryPlugin::setState(const std::string &newState) {
	enabled = ibmras::common::util::equalsIgnoreCase(newState, "on");
	if (instance) {
		instance->publishConfig();
	}
}

int MemoryPlugin::startReceiver() {

	publishConfig();
	return 0;
}

int MemoryPlugin::stopReceiver() {

	return 0;
}

bool MemoryPlugin::isEnabled() {
	return enabled;
}

/***************************************************************************************
 * Data-production/data-transfer related methods:
 * *************************************************************************************/

monitordata* MemoryPlugin::pullInt() {

	IBMRAS_DEBUG(debug, ">>>MemoryPlugin::pullInt()");

	ibmras::monitoring::plugins::j9::setEnv(&env, "Health Center (memory)", jvmF->theVM, true);

	if(!env) {
		IBMRAS_DEBUG(warning, "The JNI env was not set up");
		IBMRAS_DEBUG(debug, "<<<MemoryPlugin::pullInt[JNIenv=NULL]");
		return NULL;
	}

	monitordata* data = new monitordata;
	data->size = 0;
	data->data = NULL;

	if (isEnabled()) {
		data->persistent = false;
		data->provID = provID;
		data->sourceID = 0;
#if defined(_ZOS)
#pragma convert("ISO8859-1")
#endif
		std::string cp = ibmras::monitoring::plugins::j9::getString(env,
				"com/ibm/java/diagnostics/healthcenter/agent/dataproviders/memory/MemoryDataProvider",
				"getJMXData", "()Ljava/lang/String;");
#if defined(_ZOS)
#pragma convert(pop)
	char* memString = ibmras::common::util::createNativeString(cp.c_str());
#else
	const char* memString = cp.c_str();
#endif

		std::stringstream ss;

		ss << memString;
#if defined(_ZOS)
	ibmras::common::memory::deallocate((unsigned char**)&memString);
#endif

		ss << "physicalmemory=" << getProcessPhysicalMemorySize(env);
		ss << ",privatememory=" << getProcessPrivateMemorySize(env);
		ss << ",virtualmemory=" << getProcessVirtualMemorySize(env);
		ss << ",freephysicalmemory=" << getFreePhysicalMemorySize(env) << '\n';


		std::string memorydata = ss.str();

		jsize len = memorydata.length();
		char* asciiMem = ibmras::common::util::createAsciiString(memorydata.c_str());
		if (asciiMem) {
			data->size = len;
			data->data = asciiMem;
			IBMRAS_DEBUG(debug, "<<MEMPullSource::sourceData(DATA)");
		}
	}
	return data;
}

void MemoryPlugin::pullcompleteInt(monitordata* data) {
	IBMRAS_DEBUG(debug, ">>>MemoryPlugin::pullcompleteInt");
	if (data) {
			ibmras::monitoring::plugins::j9::hc_dealloc((unsigned char**) (&(data->data)));
			delete data;
			IBMRAS_DEBUG(debug, "<<<MemoryPlugin::pullcompleteInt[data deleted]");
	} else {
		if (env) {
			IBMRAS_DEBUG_1(debug, "Detaching thread %s", name.c_str());
			jvmF->theVM->DetachCurrentThread();
			env = NULL;
			IBMRAS_DEBUG(debug, "<<<MemoryPlugin::pullcompleteInt[thread detached]");
		}
	}
}

#if defined(LINUX)
/**
 * Opens file at /proc/<pid>/<fname> for reading.
 *
 * @param[in] env    JNIEnv of the caller.
 * @param[in] fname  Name of file to open.
 *
 * @return File descriptor of the opened file or -1 on failure.
 */
static IDATA openProcFile(JNIEnv *env, const char *fname)
{
	char proc[MAXPATHLEN];

	snprintf(proc, sizeof(proc), "/proc/%d/%s", getpid(), fname);

	return open(proc,O_RDONLY);
}

/**
 * Read proc file at /proc/<pid>/<fname> into buf of size nbytes.
 * Null-terminates the buffer so it can be treated as a string.
 *
 * @param[in]  env    JNIEnv of the caller.
 * @param[in]  fname  Name of file to open.
 * @param[out] buf    Buffer to read file into.
 * @param[in]  nbytes Size of buffer.
 *
 * @return Returns number of bytes read excluding null-terminator
 *         or -1 on failure.
 */
static IDATA readProcFile(JNIEnv *env, const char *fname, char *buf, UDATA nbytes)
{
	IDATA ret = -1;
	IDATA fd = openProcFile(env, fname);

	if (-1 != fd)
	{
		ret = 0;
		/* Read up to (nbytes - 1) bytes to save space for the null terminator. */
		while (nbytes - ret > 1)
		{
			IDATA nread = read(fd, buf + ret, nbytes - ret - 1);

			if (nread <= 0)
			break;
			ret += (UDATA)nread;
		}
		buf[ret] = '\0';
		close(fd);
	}
	return ret;
}

/**
 * Gets the running process name as null-terminated string.
 *
 * @param[in]  env         JNIEnv of the caller.
 * @param[out] name        Buffer to store name of the process.
 * @param[in]  nameLength  Length of the name buffer.
 *
 * @return Pointer to name on success or NULL on error.
 */
static char* getProcessName(JNIEnv *env, char *name, UDATA nameLength)
{
	/*
	 * Read the first line from /proc/<pid>/status and parse
	 * the process name from it.
	 *
	 * It would be nice to use prctl() with PR_GET_NAME - but
	 * it is only supported on kernel version 2.6.11 and later.
	 */
#define PROC_NAME_PREFIX "Name:\t"
	char *ret = NULL;
	char buf[128];

	if (-1 != readProcFile(env, "status", buf, sizeof(buf)))
	{
		if (0 == strncmp(buf, PROC_NAME_PREFIX, sizeof(PROC_NAME_PREFIX) - 1))
		{
			UDATA i;
			char *from = buf + sizeof(PROC_NAME_PREFIX) - 1;

			for (i = 0; (i < nameLength - 1) && ('\0' != from[i]) && ('\n' != from[i]); i++)
			{
				name[i] = from[i];
			}
			name[i] = '\0';
			ret = name;
		}
	}
	return ret;
#undef PROC_NAME_PREFIX
}

/**
 * Skips n number of space-separator fields in str. The string
 * must not begin with whitespace.
 *
 * @param[in] str Null-terminated string that will be scanned.
 * @param[in] n   Number of fields to skip, must be positive.
 *
 * @return Pointer to the location in the string after the skipped
 *         fields, or NULL if end of string was encountered.
 */
static const char* skipFields(const char *str, UDATA n)
{
	str++;
	while (('\0' != *str) && (n > 0))
	{
		if (isspace(*str))
		{
			n--;
		}
		str++;
	}
	return(n != 0 ? NULL : str);
}

/**
 * Opens /proc/<pid>/stat file and reads the field at position index
 * from the file as sscanf would. Field index must be >= 2. Returns
 * result of sscanf (i.e. number of fields read), or -1 on failure.
 *
 * @param[in]  env    JNIEnv of the caller.
 * @param[in]  index  Index of the field to be read.
 * @param[in]  format Format string for field to be read.
 * @param[out] ...    Field(s) to be read as per format.
 *
 * @return Number of fields read, or -1 on failure.
 */
static IDATA readProcStatField(JNIEnv *env, UDATA index, const char *format, ...)
__attribute__((format(scanf,3,4)));
static IDATA readProcStatField(JNIEnv *env, UDATA index, const char *format, ...)
{
	IDATA ret = -1;
	char buf[512];

	if (-1 != readProcFile(env, "stat", buf, sizeof(buf)))
	{
		/*
		 * The second field in /proc/<pid>/stat is the process name
		 * surrounded by parentheses. Unfortunately, the process
		 * name can have both spaces and parentheses in it, neither
		 * of which are escaped. Thus, to parse the file correctly,
		 * we must get the process name in order to be able to skip
		 * it in the /proc/<pid>/stat file.
		 */
		char processName[64];

		if (NULL != getProcessName(env, processName, sizeof(processName)))
		{
			char expected[128];
			size_t length = (size_t)snprintf(expected, sizeof(expected),
					"%d (%s) ", getpid(), processName);

			/* Verify that the start of the file matches what we expected. */
			if (0 == memcmp(buf, expected, length))
			{
				const char *str = skipFields(buf + length, index - 2);

				if (NULL != str)
				{
					va_list ap;
					va_start(ap, format);
					ret = vsscanf(str, format, ap);
					va_end(ap);
				}
			}
		}
	}

	return ret;
}
#endif

jlong getProcessPhysicalMemorySize(JNIEnv* env) {
#if defined(LINUX)
	/* Read rss field from /proc/<pid>/stat as per 'man proc'. */
#define RSS_FIELD_INDEX 23
	long rss;

	if (1 == readProcStatField(env, RSS_FIELD_INDEX, "%ld", &rss))
	{
		/* NOTE: This is accurate even in the context of huge pages. */
		return(jlong)rss * sysconf(_SC_PAGESIZE);
	}
#undef RSS_FIELD_INDEX
#elif defined(AIXPPC)
	/*
	 * There is no API on AIX to get the rss of the shared memory used by this process.
	 * If such an API was available, this function should return the following:
	 *
	 *   sharedRss + (pe.pi_trss + pe.pi_drss)*4096
	 *
	 * NOTE: pi_trss and pi_drss are always in 4K units regardless of pi_text_l2psize.
	 */
#elif defined(WINDOWS)
	PROCESS_MEMORY_COUNTERS info;

	info.cb = sizeof(info);
	if (0 != GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info)))
	{
		return info.WorkingSetSize;
	}

#endif
	return -1;

}

jlong getProcessPrivateMemorySize(JNIEnv* env) {

#if defined(LINUX)
	/*
	 * Read shared field from /proc/<pid>/statm as per 'man proc'.
	 * Return difference between virtual memory size and shared.
	 */
#define SHARED_FIELD_INDEX 2
	char buf[512];

	if (-1 != readProcFile(env, "statm", buf, sizeof(buf)))
	{
		const char *str = skipFields(buf, SHARED_FIELD_INDEX);

		if (NULL != str)
		{
			long shared;

			if (1 == sscanf(str, "%ld", &shared))
			{
				jlong vsize = getProcessVirtualMemorySize(env);

				if (-1 != vsize)
				{
					jlong priv = vsize - ((jlong)shared * sysconf(_SC_PAGESIZE));

					return(priv > 0 ? priv : -1);
				}
			}
		}
	}
#undef SHARED_FIELD_INDEX
#elif defined(AIXPPC)
	struct procentry64 pe;
	pid_t pid = getpid();

	if (1 == getprocs64((struct procentry64*)&pe, sizeof(pe), NULL, 0, &pid, 1))
	{
		/* NOTE: pi_dvm is always in 4K units regardless of pi_data_l2psize. */
		jlong size = (jlong)pe.pi_tsize + (jlong)pe.pi_dvm * 4096;

		return(size > 0 ? size : -1);
	}
#elif defined(WINDOWS)

	IBMRAS_DEBUG(debug, ">>MEMPullSource::getProcessPrivateMemorySizeImpl()");

	PROCESS_MEMORY_COUNTERS_EX procMemCount;

	bool result = GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&procMemCount), sizeof(PROCESS_MEMORY_COUNTERS_EX));

	if(result) {
		return procMemCount.PrivateUsage > 0 ? procMemCount.PrivateUsage : -1;
	}
	return -1;

#endif
	IBMRAS_DEBUG(debug, "<<MEMPullSource::getProcessPrivateMemorySizeImpl()[ERROR]");
	return -1;
}

jlong getProcessVirtualMemorySize(JNIEnv* env) {
#if defined(LINUX)
	/* Read vsize field from /proc/<pid>/stat as per 'man proc'. */
#define VSIZE_FIELD_INDEX 22
	unsigned long vsize;

	if (1 == readProcStatField(env, VSIZE_FIELD_INDEX, "%lu", &vsize))
	{
		return(jlong)(vsize > 0 ? vsize : -1);
	}
#undef VSIZE_FIELD_INDEX
#elif defined(AIXPPC)
	/* There is no API on AIX to get shared memory usage for the process. If such an
	 * API existed, we could return getProcessPrivateMemorySize() + sharedSize here.
	 *
	 * Note: Iterating through /proc/<pid>/map and looking at the pages that are
	 * not marked MA_SHARED does not account for shared code pages when in fact
	 * command-line AIX utilities (such as svmon) do show that pages are shared.
	 */
#elif defined(WINDOWS)
	PROCESS_MEMORY_COUNTERS info;

	info.cb = sizeof(info);
	if (0 != GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info)))
	{
		return(jlong)info.PagefileUsage;
	}
#endif
	return -1;
}

jlong getFreePhysicalMemorySize(JNIEnv* env) {
#if defined(LINUX)
	/* NOTE: This is accurate even in the context of huge pages. */
	return(jlong)sysconf(_SC_AVPHYS_PAGES) * sysconf(_SC_PAGESIZE);
#elif defined(AIXPPC)
	/* NOTE: This works on AIX 5.3 and later. */
	IDATA numPageSizes = vmgetinfo(NULL, VMINFO_GETPSIZES, 0);

	if (numPageSizes > 0)
	{
		psize_t *pageSizes = (psize_t*)__alloca(numPageSizes*sizeof(psize_t));
		IDATA numPageSizesRetrieved = vmgetinfo(pageSizes, VMINFO_GETPSIZES, numPageSizes);

		if (numPageSizes == numPageSizesRetrieved)
		{
			jlong size = 0;
			IDATA i;

			for (i = 0; i < numPageSizes; i++)
			{
				struct vminfo_psize pageSize;

				pageSize.psize = pageSizes[i];
				if (0 == vmgetinfo(&pageSize, VMINFO_PSIZE, sizeof(pageSize)))
				{
					size += (jlong)pageSize.psize * pageSize.numfrb;
				}
			}
			return(size > 0 ? size : -1);
		}
	}
	return -1;
#elif defined(WINDOWS)
	MEMORYSTATUSEX statex;

	statex.dwLength = sizeof(statex);
	if (0 != GlobalMemoryStatusEx(&statex))
	{
		return statex.ullAvailPhys;
	}
	return -1;
#else
	return -1;
#endif
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
	ibmras::monitoring::agent::Agent* agent = ibmras::monitoring::agent::Agent::getInstance();
	std::string enableProp = agent->getAgentProperty("data.memory");

	if (enableProp == "on" || enableProp == "") {
		enabled = true;
		instance->startReceiver();
	} else {
		enabled = false;
		instance->stopReceiver();
	}
	instance->publishConfig();
	return 0;
}

int stopWrapper() {
	return instance->stopReceiver();
}

const char* getMemVersion() {
	return "1.0";
}

}//memory
}//j9
}//plugins
}//monitoring
}//ibmras
