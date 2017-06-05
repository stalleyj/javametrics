/**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */

/*
 * Functions that control thread behaviour
 */

#pragma longname
#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED 1
#define _UNIX03_THREADS
#define _OPEN_SYS
#define _OPEN_SYS_TIMED_EXT 1
#include <pthread.h>
#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#include "ibmras/common/port/ThreadData.h"
#include "ibmras/common/port/Semaphore.h"
#include "ibmras/common/logging.h"

extern "C" int IEAVIFAT();
extern "C" int IEAVIFAF();

namespace ibmras {
namespace common {
namespace port {

#define IEAVIFA_SUCCESS	0x00000000
#define IEAVIFA_RCMASK 	0x000000FF
#define IEAVIFA_NOTAUTHLIB 0x00000708

IBMRAS_DEFINE_LOGGER("Port");


int ifa_switch(void) {
	/* check for to see if we have IFA support */
	int returnCode;

	/* call switch to, store return code in returnCode */
	returnCode = IEAVIFAT();

	if (IEAVIFA_SUCCESS == returnCode) {
		IBMRAS_DEBUG(fine, "Switch to IFA processor successful");
		return 1;
	} else if ((returnCode & IEAVIFA_RCMASK) == 0x8) {
		/* error */
		if (returnCode == IEAVIFA_NOTAUTHLIB) {
			IBMRAS_DEBUG(fine, "Unable to switch to IFA processor - issue 'extattr +a libhealthcenter.so'");
		} else {
			IBMRAS_DEBUG_1(fine, "Error switching to IFA processor rc: %d", returnCode);
		}
		return 0;
	} else
		IBMRAS_DEBUG_1(fine, "IFA Error: unexpected return code %d from IFA switch service", returnCode);
	return 0;
}

extern "C" void* wrapper(void *params) {
	IBMRAS_DEBUG(fine,"in thread.cpp->wrapper");
    // zaap enable if available
	ifa_switch();
	ThreadData* data = reinterpret_cast<ThreadData*>(params);
	return data->getCallback()(data);
}

uintptr_t createThread(ThreadData* data) {
	IBMRAS_DEBUG(fine,"in thread.cpp->createThread");
	pthread_t thread;
	return pthread_create(&thread, NULL, wrapper, data);
}

void exitThread(void *val) {
	IBMRAS_DEBUG(fine,"in thread.cpp->exitThread");
	// thread ending, switch zaap off
	IEAVIFAF();
	pthread_exit(NULL);
}

void sleep(uint32 seconds) {
	IBMRAS_DEBUG(fine,"in thread.cpp->sleep");
	::sleep(seconds); /* configure the sleep interval */
}

void stopAllThreads() {
	IBMRAS_DEBUG(fine,"in thread.cpp->stopAllThreads");
}

int key_increment = 0;

int initsem(int nsems) {
	int semid = -1;

	time_t seconds;
	seconds = time(NULL);
	key_t key = seconds;

	// Retry until we can get a unique semaphore.
	for (int i = 1; i < 100; i++) {
		IBMRAS_DEBUG_1(debug, "getting semaphore for key %d", (int)key);
		semid = semget(key, nsems, IPC_CREAT | IPC_EXCL | 0666);
		if (semid == -1 && errno == EEXIST) {

			IBMRAS_DEBUG_1(debug, "semaphore for key %d already exists, retrying", (int)key);
			key = key - 1;
		} else {
			break;
		}
	}

	return semid;
}

int sem_initialize(int *semid, int value) {
	int ret = semctl(*semid, 0, SETVAL, value);
	return ret;
}

int sem_init(int *sem, int pshared, unsigned int value) {
	int ret = -1;
	if (value > INT_MAX) {
		errno = EINVAL;
		return ret;
	}
	if ((*sem = initsem(1)) == -1) {
		return -1;
	} else
		ret = sem_initialize(sem, value);
	if (ret == -1) {
		return -1;
	}
	return ret;
}

int sem_destroy(int *semid) {
	int ret = semctl(*semid, 0, IPC_RMID);
	if (ret == -1) {
		return -1;
	}
	return ret;
}

int sem_post(int *semid) {
	struct sembuf sb;
	sb.sem_num = 0;
	sb.sem_op = 1;
	sb.sem_flg = 0;
	if (semop(*semid, &sb, 1) == -1) {
		return -1;
	}
	return 0;
}

int sem_wait(int *semid) {
	struct sembuf sb;
	sb.sem_num = 0;
	sb.sem_op = -1;
	sb.sem_flg = 0;
	if (semop(*semid, &sb, 1) == -1) {
		return -1;
	}
	return 0;
}

int sem_timedwait(int *semid, struct timespec *t) {
	struct sembuf sb;
	sb.sem_num = 0;
	sb.sem_op = -1;
	sb.sem_flg = 0;
	if (__semop_timed(*semid, &sb, 1, t) == -1) {
		return -1;
	}
	return 0;
}

Semaphore::Semaphore(uint32 initial, uint32 max) {
	IBMRAS_DEBUG(fine,"in thread.cpp creating CreateSemaphoreA");
	handle = new int*;
	int result;
	result = sem_init(reinterpret_cast<int*>(handle), 0, initial);
	if (result) {
		IBMRAS_DEBUG_1(warning, "Failed to create semaphore : error code %d", result);
		handle = NULL;
	}
}

void Semaphore::inc() {
	IBMRAS_DEBUG(finest, "Incrementing semaphore ticket count");
	if (handle) {
		sem_post(reinterpret_cast<int*>(handle));
	}
}

bool Semaphore::wait(uint32 timeout) {
	int result;
	struct timespec t;

	while (!handle) {
		sleep(timeout); /* wait for the semaphore to be established */
	}

	t.tv_sec = timeout; /* configure the sleep interval */
	t.tv_nsec = 0;

	result = sem_timedwait(reinterpret_cast<int*>(handle), &t);
	if (!result) {
		IBMRAS_DEBUG(finest, "semaphore posted");
		return true;
	} 

    IBMRAS_DEBUG(finest, "possible semaphore timeout");
	return (errno != EAGAIN);
}

Semaphore::~Semaphore() {
	sem_destroy(reinterpret_cast<int*>(handle));
	delete (int*) handle;
}

}
}
} /* end namespace port */
