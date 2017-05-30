 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */

#ifndef ibmras_monitoring_plugins_omr_tracedataprovider_h
#define ibmras_monitoring_plugins_omr_tracedataprovider_h

#include "ibmras/monitoring/Plugin.h"
#include "omr.h"
#include "omragent.h"
#include "ibmras/vm/omr/healthcenter.h"
#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(WINDOWS)
#include <winsock2.h>
#define JLONG_FMT_STR "%I64d"
#else /* Unix platforms */
#define _OE_SOCKETS
#define JLONG_FMT_STR "%lld"
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif



namespace plugins {
	namespace omr {
		namespace trace {

			class TraceDataProvider : ibmras::monitoring::Plugin {
			public:
				static TraceDataProvider* getInstance(omrRunTimeProviderParameters oRTPP);
				static TraceDataProvider* getInstance();
				TraceDataProvider(omrRunTimeProviderParameters oRTPP);
				virtual ~TraceDataProvider() {};  //destructor
			private:

			};

		} /* end namespace trace */
	} /* end namespace omr */
} /* end namespace plugins */

#endif /* ibmras_monitoring_plugins_omr_tracedataprovider_h */
