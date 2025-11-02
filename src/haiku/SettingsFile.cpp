
#include "SettingsFile.h"

#include <Directory.h>
#include <fs_info.h>
#include <FindDirectory.h>
#include <Path.h>

#include <cctype>
#include <functional> 
#include <locale>
#include <vector>


std::string
ltrim (std::string const &s)
{
	return std::string(std::find_if(s.cbegin(), s.cend(), [](uint8 c) 
	{
		return ! std::isspace(c);
	}
	), s.cend());
}


std::string
rtrim (std::string const &s)
{
	return std::string(s.cbegin(), std::find_if(s.crbegin(), s.crend(), [](uint8 c)
	{
		return ! std::isspace(c); 
	}
	).base());
}


void 
trim(std::string &s)
{
	ltrim(s);
	rtrim(s);
}


std::vector<std::string> 
explode(std::string const &delimeter, std::string const &s, int limit)
{
	std::vector<std::string> r;

	if(! s.empty()) {
		if(limit >= 0) {
			if(limit == 0) {
				limit = 1;
			}

			std::size_t first = 0;
			std::size_t last  = s.find(delimeter);

			while(last != std::string::npos) {

				if(--limit == 0) {
					break;
				}

				r.push_back(s.substr(first, last - first));
				first = last + delimeter.size();
				last  = s.find(delimeter, last + delimeter.size());
			}

			r.push_back(s.substr(first));
		} else {
			std::size_t first = 0;
			std::size_t last  = s.find(delimeter);

			while(last != std::string::npos) {
				r.push_back(s.substr(first, last - first));
				first = last + delimeter.size();
				last  = s.find(delimeter, last + delimeter.size());
			}

			r.push_back(s.substr(first));
			
			while(limit < 0) {
				r.pop_back();
				++limit;
			}
		}
	}
	
	return r;
}


inline std::vector<std::string> 
explode (std::string const &delimeter, std::string const &s)
{
	return explode(delimeter, s, std::numeric_limits<int>::max());
}


SettingsFile::SettingsFile() 
{
	std::cout  << __PRETTY_FUNCTION__ << std::endl;
	BPath path;
	BDirectory *dir;
		
	find_directory(B_USER_SETTINGS_DIRECTORY, &path, false);
	filename_ = path.Path();
	dir = new BDirectory(filename_.c_str());
	dir->CreateDirectory("Pretendo", NULL);
	filename_ += "/Pretendo/pretendo_settings";

	Load();
}


SettingsFile::~SettingsFile() 
{

}


bool 
SettingsFile::Load()
{
	std::cout << "Loading settings from file..." << std::endl;
	
	std::ifstream file(filename_.c_str());
		
	if (! file) {
		// file does not exist, make a new one with some defaults
		std::cout << "Couldn't load file. Creating new one..." << std::endl;
		
		NewSection("App Settings");
		NewKey("App Settings", std::make_pair("ShowOpenOnLoad", "false"));
		NewKey("App Settings", std::make_pair("AutoRun", "false"));
		NewKey("App Settings", std::make_pair("SleepOnLoseFocus", "false"));
		NewKey("App Settings", std::make_pair("ROMDirectory", "./roms"));
		
		Save();
		return false;
	}

	std::string linebuffer;
	std::string current_section;
	unsigned int line_number = 0;

	while (std::getline(file, linebuffer)) {
	
		trim(linebuffer);
	
		++line_number;
		if (linebuffer.empty()) {
			continue;
		}

		if (linebuffer[0] == '[') {
		
			// TODO: handle if there is junk after the closing ']' character
			size_t const end = linebuffer.find_last_of(']');

			if (end != std::string::npos) {
				current_section = linebuffer.substr(1, end - 1);
				NewSection(current_section);
			} else {
				std::cerr << "[SettingsFile::Load] Error on line " << line_number << std::endl;
				continue;
			}
		} else if (linebuffer[0] == '#') {
			// skip comments
			continue;
		} else {			
			if (current_section.empty()) {
				std::cerr << "Error: every configuration option must be in a section" << std::endl;
				continue;
			}
			
			std::vector<std::string> const values = explode("=", linebuffer);
			
			if (values.size() != 2) {
				std::cerr << "Error: Every key must have exactly one value" << line_number << std::endl;
				continue;
			}

			std::string key   = values[0];
			std::string value = values[1];
			
			trim(key);
			trim(value);

			if (key.empty()) {
				std::cerr << "Error: bad key on line " << line_number << std::endl;
				continue;
			}

			NewKey(current_section, std::make_pair(key, value));
		}
	}
	
	return true;
}


bool
SettingsFile::Save()
{

	std::cout << "Saving Settings..." << std::endl;
	std::ofstream file(filename_.c_str(), std::ios::trunc);

	if (! file) {
		std::cerr << "[SettingsFile::Save] Error: couldn't open file for writing" << std::endl;
		// TODO: throw exception or something equally creative
		return false;
	}

	for (auto ci = sections_.begin(); ci != sections_.end(); ++ci) {

		file << "\n[" << ci->first << "]" << std::endl;

		for (auto ki = sections_[ci->first].begin(); ki != sections_[ci->first].end(); ++ki) {
			if (ki->first.empty() || ki->second.empty()) {
				continue;
			}

			file << ki->first << "=" << ki->second << std::endl;
		}
	}

	return true;
}


bool
SettingsFile::DeleteSection (std::string const &section)
{

	std::cout << "DeleteSection -> " << section << std::endl;
	auto it = sections_.find(section);

	if (it == sections_.end()) {
		return false;
	}
	
	sections_.erase(it);
	return true;
}


bool 
SettingsFile::NewSection (std::string const &section)
{
	std::cout << "NewSection: " << section << std::endl;

	if (section.empty()) {
		return false;
	}
	
	auto it = sections_.insert(std::make_pair(section, section_type()));

	if (! it.second) {
		std::cout << "Section: " << section << " already in list." << std::endl;
		return false;
	}
	
	return true;
}


bool 
SettingsFile::NewKey (std::string const &section, std::pair<std::string, std::string> const &key) 
{
	if (section.empty()) {
		return false;
	}

	if (sections_.find(section) == sections_.end()) {
		return false;
	}

	std::cout << "NewKey -> adding: " << key.first << ", " << key.second << std::endl;
	
	auto it = sections_[section].insert(key);
	if (! it.second) {
		std::cout << "key " << key.first << " already exists." << std::endl;
	}
	
	return true;
}


bool
SettingsFile::DeleteKey(std::string const &section, std::string const &keyName)
{
	if(section.empty()) {
		return false;
	}

	for (auto it = sections_[section].begin(); it != sections_[section].end(); ++it){
		if (it->first == keyName) {
			std::cout << "DeleteKey -> " << keyName << std::endl;
			sections_[section].erase(it);
			return true;
		}
	}

	return false;
}
