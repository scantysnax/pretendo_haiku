
#include "SimpleMutex.h"


SimpleMutex::SimpleMutex (char const *debugName)
{
	fLocker = create_sem(1, debugName);
}


SimpleMutex::~SimpleMutex()
{
	delete_sem(fLocker);
}


bool
SimpleMutex::Lock() const
{
	status_t error = acquire_sem(fLocker);
	
	return (error == B_NO_ERROR) ? true : false;
}


bool
SimpleMutex::Unlock() const
{
	status_t error = release_sem(fLocker);
	
	return (error == B_NO_ERROR) ? true : false;
}
