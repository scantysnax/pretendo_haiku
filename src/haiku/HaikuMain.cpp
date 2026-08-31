
#include <ctime>
#include "PretendoApp.h"


// -----------------------------------------------------------------------------
// main
//
// Application entry point.
//
// Seeds the standard pseudo-random number generator, creates the Pretendo
// application object, and enters the Haiku application event loop.  When the
// application exits, the global application object is destroyed.
//
// Parameters:
//   None.
//
// Returns:
//   Zero on normal application termination.
// -----------------------------------------------------------------------------
int
main (void)
{
	srand(static_cast<unsigned int>(time(0)));
	PretendoApp *app = new PretendoApp;
	app->Run();
	delete be_app;
	
	return 0;
}
