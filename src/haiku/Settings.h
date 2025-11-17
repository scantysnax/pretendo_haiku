
#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <string>

namespace Settings {

void load();
void save();

std::string configDirectory();
std::string cacheDirectory();

}

#endif
