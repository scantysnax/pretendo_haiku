
#ifndef _SIMPLE_MUTEX_H_
#define _SIMPLE_MUTEX_H_

#include <OS.h>


class SimpleMutex
{
	public:
			SimpleMutex (char const *debugName);
	virtual ~SimpleMutex();
	
	public:
	bool Lock() const;
	bool Unlock() const;
	sem_id Locker() const { return fLocker; }
	
	private:
	sem_id fLocker;
};

#endif // _SIMPLE_MUTEX_H_
