
#include "Mutex.h"


Mutex::Mutex (char const *debugName, bigtime_t timeOut)
{
	fLocker = create_sem(kThreadCount, debugName);
	fTimeOut = timeOut;
}


Mutex::~Mutex()
{
	delete_sem(fLocker);
}


bool
Mutex::Lock() const
{
	status_t error;

	do {
		error = acquire_sem_etc(fLocker, kThreadCount, B_RELATIVE_TIMEOUT, fTimeOut);
	} while (error == B_INTERRUPTED);
	
	return (error == B_NO_ERROR) ? true : false;
}


bool
Mutex::Unlock() const
{
	status_t error = release_sem_etc(fLocker, kThreadCount, B_DO_NOT_RESCHEDULE);
	
	return (error == B_NO_ERROR) ? true : false;
}
