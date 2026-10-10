
#ifndef _ROM_DATABASE_H_
#define _ROM_DATABASE_H_

#include <string>

#include <libxml2/libxml/tree.h>


class ROMDatabase {
	
	public:
	struct Entry {
		bool found = false;

		std::string game_name;
		std::string board_type;
		std::string chip_type;

		std::string pin3_function;
		std::string pin4_function;
	};

	public:
	static Entry LookupBySHA1(const std::string &database_path, const std::string &sha1);

	private:
	static xmlNodePtr FindCartridge(xmlNodePtr root, const xmlChar *search_key,
									const xmlChar *search_value,
									xmlNodePtr *game_out);

	static xmlNodePtr FindCartridgeInGame(xmlNodePtr game, const xmlChar *search_key,
										  const xmlChar *search_value);

	private:
	static std::string GetProperty(xmlNodePtr node, const char *name);
};


#endif 	// _ROM_DATABASE_H_
