
#include "Settings.h"


namespace Settings {

#if __HAIKU__

std::string
configDirectory()
{
	std::string path = "/boot/home/config/settings/Pretendo";
	return path;
}


std::string
cacheDirectory()
{	
	std::string path = "/boot/home/config/settings/Pretendo/pretendo_cache";
	return path;
}


std::string
romDatabasePath()
{
	std::string path = "/boot/home/config/settings/Pretendo/nescarts.xml";
	return path;
}


void
load()
{
	
}


void
save()
{
	
}

#else

#error "Please implement necessary paths for for your platform"

#endif

};
