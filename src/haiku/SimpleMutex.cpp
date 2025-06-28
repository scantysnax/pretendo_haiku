
#include "SimpleMutex.h"


SimpleMutex::SimpleMutex (char const *debugName)
{
	fMutex = create_sem(1, debugName);
}


SimpleMutex::~SimpleMutex()
{
	delete_sem (fMutex);
}


status_t
SimpleMutex::Lock()
{
	return acquire_sem(fMutex);
}


status_t
SimpleMutex::Unlock()
{
	return release_sem(fMutex);
}
