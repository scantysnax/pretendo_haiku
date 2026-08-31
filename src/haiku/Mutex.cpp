
#include "Mutex.h"


// -----------------------------------------------------------------------------
// Mutex::Mutex
//
// Creates the semaphore used by this Mutex and stores the default lock timeout.
//
// The semaphore is initialized with kThreadCount available permits.
//
// Parameters:
//   debugName - Name assigned to the Haiku semaphore for debugging.
//   timeOut   - Default relative timeout used by the parameterless Lock().
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
Mutex::Mutex(char const *debugName, bigtime_t timeOut)
{
	fLocker = create_sem(kThreadCount, debugName);
	fTimeOut = timeOut;
}


// -----------------------------------------------------------------------------
// Mutex::~Mutex
//
// Deletes the semaphore owned by this Mutex.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
Mutex::~Mutex()
{
	delete_sem(fLocker);
}


// -----------------------------------------------------------------------------
// Mutex::Lock
//
// Acquires the mutex using the default timeout supplied when the Mutex was
// constructed.
//
// Parameters:
//   None.
//
// Returns:
//   true if the mutex was acquired.
// -----------------------------------------------------------------------------
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


// -----------------------------------------------------------------------------
// Mutex::Unlock
//
// Releases the semaphore permit previously acquired by Lock().  Rescheduling is
// suppressed during the release so the caller may continue executing without
// immediately yielding to a waiting thread.
//
// Parameters:
//   None.
//
// Returns:
//   true if the semaphore permit was released successfully.
// -----------------------------------------------------------------------------
bool
Mutex::Unlock() const
{
	status_t error = release_sem_etc(fLocker, kThreadCount, B_DO_NOT_RESCHEDULE);

	return (error == B_NO_ERROR);
}


