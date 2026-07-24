
#include "Mutex.h"


Mutex::Mutex(char const* debugName, bigtime_t timeOut)
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
	return Lock(fTimeOut);
}


// -----------------------------------------------------------------------------
// Mutex::Lock
//
// Acquires the mutex, waiting up to the supplied relative timeout.  Interrupted
// waits are retried so transient signals do not cause a false lock failure.
//
// Parameters:
//   timeOut - Relative timeout in microseconds, or B_INFINITE_TIMEOUT.
//
// Returns:
//   true if the mutex was acquired.
// -----------------------------------------------------------------------------
bool
Mutex::Lock(bigtime_t timeOut) const
{
	status_t error;

	do {
		error = acquire_sem_etc(fLocker, kThreadCount, B_RELATIVE_TIMEOUT, timeOut);
	} while (error == B_INTERRUPTED);
	
	return error == B_NO_ERROR;
}


bool
Mutex::Unlock() const
{
	status_t error = release_sem_etc(fLocker, kThreadCount, B_DO_NOT_RESCHEDULE);
	
	return error == B_NO_ERROR;
}

