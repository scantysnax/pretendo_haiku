
#ifndef _SIMPLE_MUTEX_H_
#define _SIMPLE_MUTEX_H_

#include <OS.h>


constexpr uint32 kThreadCount = 1;
constexpr bigtime_t kTimeOut = 1000000LL;


class SimpleMutex
{
	public:
			SimpleMutex (char const *debugName, bigtime_t timeOut = B_INFINITE_TIMEOUT);
	virtual ~SimpleMutex();
	
	public:
	bool Lock() const;
	bool Unlock() const;
	sem_id Locker() const { return fLocker; }
	
	private:
	sem_id fLocker;
	bigtime_t fTimeOut;
};

#endif // _SIMPLE_MUTEX_H_
