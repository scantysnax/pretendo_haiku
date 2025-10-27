
#ifndef _SETTINGS_FILE_H_
#define _SETTINGS_FILE_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <typeinfo>
#include <map>


class SettingsFile {
public:
	SettingsFile();
	~SettingsFile();
	
private:
	SettingsFile(const SettingsFile &) = delete;
	SettingsFile &operator=(const SettingsFile &) = delete;

private:
	typedef std::map<std::string, std::string> section_type;

public:
	bool Load();
	bool Save();

public:
	bool NewSection(const std::string &section);
	bool DeleteSection(const std::string &section);
	bool NewKey(const std::string &section, const std::pair<std::string, std::string> &key);
	bool DeleteKey(const std::string &section, const std::string &keyName);

public:
	template <class T>
	bool ReadKey(const std::string &section, const std::string &key, T &container) {

		if(section.empty() || key.empty()) {
			return false;
		}
				
		auto it = sections_.find(section);

		if(it == sections_.end()) {
			std::cout << "cant find section" << std::endl;
			return false;
		}

		const section_type &kvm = it->second;
		for(auto kvi = kvm.begin(); kvi != kvm.end(); ++kvi) {
			std::cout << key << ", " << kvi->first << std::endl;

			if(key == kvi->first) {
				container = (kvi->second);
				return true;
			}
		}

		return false;
	}

	template <class T>
	bool WriteKey(const std::string &section, const std::string &key, T value) {

		if(section.empty() || key.empty()) {
			return false;
		}
		
		auto it = sections_.find(section);

		if(it == sections_.end()) {
			return false;
		}

		std::ostringstream oss;
		if(typeid(T) == typeid(double)) {
			oss << std::setprecision(4) << value;
		} else {
			oss << value;
		}

		section_type &kvm = it->second;
		for(auto kvi = kvm.begin(); kvi != kvm.end(); ++kvi) {
			std::cout << key << ", " << kvi->first << std::endl;
			if(key == kvi->first) {
				kvi->second = oss.str();
				return true;
			}
		}

		return false;
	}

private:
	std::string                         filename_;
	std::map<std::string, section_type> sections_;
};


#endif	// _SETTINGS_FILE_H_
