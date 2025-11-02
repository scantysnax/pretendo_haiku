
#include <iostream>

#include "Settings.h"


namespace Settings {

std::string romDirectory;
int zoomFactor;
bool showSprites;

std::string configFile() {
	std::cout << __PRETTY_FUNCTION__ << std::endl;
	
	std::string path = "/boot/home/config/settings/Pretendo/pretendo_settings";
	return path;
}

std::string cacheDirectory() {
	std::cout << __PRETTY_FUNCTION__ << std::endl;
	
	std::string path = "/boot/home/config/settings/Pretendo/pretendo_cache";
	return path;
}

std::string homeDirectory() {
	std::cout << __PRETTY_FUNCTION__ << std::endl;
	
	std::string path = "/boot/home";
	return path;
}

void load() {
	std::cout << __PRETTY_FUNCTION__ << std::endl;	

#if 0
	auto filename = QString::fromStdString(configFile());
	QSettings settings(filename, QSettings::IniFormat);

	romDirectory = settings.value("romDirectory", QString::fromStdString(homeDirectory())).toString().toStdString();
	zoomFactor   = settings.value("zoomFactor", 2).toInt();
	showSprites  = settings.value("showSprites", true).toBool();
#endif
}

void save() {
	std::cout << __PRETTY_FUNCTION__ << std::endl;	

#if 0
	auto filename = QString::fromStdString(configFile());
	QSettings settings(filename, QSettings::IniFormat);

	settings.setValue("romDirectory", QString::fromStdString(romDirectory));
	settings.setValue("zoomFactor", zoomFactor);
	settings.setValue("showSprites", showSprites);

	settings.sync();
#endif

}


}
