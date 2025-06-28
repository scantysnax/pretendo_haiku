
#ifndef _SIMPLE_MUTEX_H_
#define _SIMPLE_MUTEX_H_

#include <OS.h>
#include <MediaDefs.h>

#include <cstring>


class SimpleMutex
{
	public:
	SimpleMutex (char const *debugName);
	virtual ~SimpleMutex();
	
	public:
	status_t Lock();
	status_t Unlock();
	sem_id Mutex() const { return fMutex; }
	
	private:
	sem_id fMutex;
};

#endif // _SIMPLE_MUTEX_H_
