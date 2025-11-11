
#include <iostream>

#include "Settings.h"


namespace Settings {

std::string configDirectory() {
	std::cout << __PRETTY_FUNCTION__ << std::endl;
	
	std::string path = "/boot/home/config/settings/Pretendo";
	return path;
}


std::string cacheDirectory() {
	std::cout << __PRETTY_FUNCTION__ << std::endl;
	
	std::string path = "/boot/home/config/settings/Pretendo/pretendo_cache";
	return path;
}


void load() {
	std::cout << __PRETTY_FUNCTION__ << std::endl;	
}

void save() {
	std::cout << __PRETTY_FUNCTION__ << std::endl;	
}


};
