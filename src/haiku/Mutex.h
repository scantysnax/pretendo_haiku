#ifndef _MUTEX_H_
#define _MUTEX_H_

#include <OS.h>

constexpr uint32 kThreadCount = 1;

class Mutex
{
	public:
			Mutex(char const* debugName, bigtime_t timeOut = B_INFINITE_TIMEOUT);
	virtual ~Mutex();
	
	public:
	bool Lock() const;
	bool Lock (bigtime_t timeOut) const;
	bool Unlock() const;

	sem_id Locker() const
	{ 
		return fLocker; 
	}
	
	private:
	sem_id fLocker;
	bigtime_t fTimeOut;
};


#endif // _MUTEX_H_

