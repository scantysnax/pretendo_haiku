
#include "SimpleMutex.h"


SimpleMutex::SimpleMutex (char const *debugName)
{
	fMutex = create_sem(1, debugName);
}


SimpleMutex::~SimpleMutex()
{
	delete_sem (fMutex);
}


bool
SimpleMutex::Lock()
{
	status_t error = acquire_sem(fMutex);
	
	return (error == B_NO_ERROR) ? true : false;
}


bool
SimpleMutex::Unlock()
{
	status_t error = release_sem(fMutex);
	
	return (error == B_NO_ERROR) ? true : false;
}
