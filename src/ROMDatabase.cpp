#include "ROMDatabase.h"

#include <libxml2/libxml/parser.h>

#include <algorithm>
#include <cctype>
#include <string>


namespace {

// -----------------------------------------------------------------------------
// normalize_sha1
//
// Normalizes a SHA-1 string to uppercase so database lookups are insensitive
// to the case used by the caller.
//
// Parameters:
//   value - SHA-1 string to normalize.
//
// Returns:
//   Uppercase copy of the supplied string.
// -----------------------------------------------------------------------------
std::string
normalize_sha1 (const std::string &value)
{
	std::string result = value;

	std::transform(result.begin(), result.end(), result.begin(),
				   [](unsigned char c) {
						return static_cast<char>(std::toupper(c));
					});

	return result;
}


// -----------------------------------------------------------------------------
// is_vrc_chip
//
// Determines whether a chip type represents one of Konami's VRC mapper chips.
//
// This deliberately ignores support logic such as 74xx139, 74xx20, MM1026,
// and similar auxiliary chips that may appear before the actual mapper chip in
// nescarts.xml.
//
// Parameters:
//   type - Chip type string from the database.
//
// Returns:
//   true if the chip type begins with "VRC".
// -----------------------------------------------------------------------------
bool
is_vrc_chip (const std::string &type)
{
	return type.size() >= 3
		&& type[0] == 'V'
		&& type[1] == 'R'
		&& type[2] == 'C';
}

}


// -----------------------------------------------------------------------------
// GetProperty
//
// Returns a named XML property as a std::string.
//
// Parameters:
//   node - XML node containing the requested property.
//   name - Property name.
//
// Returns:
//   Property value, or an empty string if the property is absent.
// -----------------------------------------------------------------------------
std::string
ROMDatabase::GetProperty (xmlNodePtr node, const char *name)
{
	if (node == nullptr || name == nullptr) {
		return {};
	}

	xmlChar *value = xmlGetProp(node, reinterpret_cast<const xmlChar *>(name));

	if (value == nullptr) {
		return {};
	}

	std::string result(reinterpret_cast<const char *>(value));

	xmlFree(value);

	return result;
}


// -----------------------------------------------------------------------------
// FindCartridgeInGame
//
// Searches the cartridge entries beneath a single <game> node for a matching
// attribute/value pair.
//
// Parameters:
//   game         - <game> XML node to search.
//   search_key   - Cartridge attribute name.
//   search_value - Cartridge attribute value.
//
// Returns:
//   Matching <cartridge> node, or nullptr if none is found.
// -----------------------------------------------------------------------------
xmlNodePtr
ROMDatabase::FindCartridgeInGame (xmlNodePtr game, const xmlChar *search_key, const xmlChar *search_value)
{
	if (game == nullptr || search_key == nullptr || search_value == nullptr) {
		return nullptr;
	}

	for (xmlNodePtr cartridge = game->children; cartridge != nullptr; cartridge = cartridge->next) {
		if (cartridge->type != XML_ELEMENT_NODE) {
			continue;
		}

		if (xmlStrcmp(cartridge->name, reinterpret_cast<const xmlChar *>("cartridge")) != 0) {
			continue;
		}

		xmlChar *value = xmlGetProp(cartridge, search_key);

		if (value == nullptr) {
			continue;
		}

		const bool match = xmlStrcasecmp(value, search_value) == 0;

		xmlFree(value);

		if (match) {
			return cartridge;
		}
	}

	return nullptr;
}


// -----------------------------------------------------------------------------
// FindCartridge
//
// Searches the complete ROM database for a cartridge whose requested attribute
// matches the supplied value.
//
// Parameters:
//   root         - Root <database> XML node.
//   search_key   - Cartridge attribute name.
//   search_value - Cartridge attribute value.
//   game_out     - Receives the owning <game> node when a match is found.
//
// Returns:
//   Matching <cartridge> node, or nullptr if none is found.
// -----------------------------------------------------------------------------
xmlNodePtr
ROMDatabase::FindCartridge (xmlNodePtr root, const xmlChar *search_key, const xmlChar *search_value,
							xmlNodePtr *game_out)
{
	if (game_out != nullptr) {
		*game_out = nullptr;
	}

	if (root == nullptr || search_key == nullptr || search_value == nullptr) {
		return nullptr;
	}

	for (xmlNodePtr game = root->children; game != nullptr; game = game->next) {
		if (game->type != XML_ELEMENT_NODE) {
			continue;
		}

		if (xmlStrcmp(game->name, reinterpret_cast<const xmlChar *>("game")) != 0) {
			continue;
		}

		xmlNodePtr cartridge = FindCartridgeInGame(game, search_key, search_value);

		if (cartridge != nullptr) {
			if (game_out != nullptr) {
				*game_out = game;
			}

			return cartridge;
		}
	}

	return nullptr;
}


// -----------------------------------------------------------------------------
// LookupBySHA1
//
// Looks up a cartridge in a NES cartridge XML database by SHA-1 and extracts
// mapper-relevant board and chip information.
//
// The database path is supplied by the caller so this code remains independent
// of any particular operating system or frontend.
//
// When a board contains several <chip> elements, auxiliary logic chips are
// ignored and the Konami VRC mapper chip is selected.
//
// Parameters:
//   database_path - Path to the cartridge XML database.
//   sha1          - SHA-1 hash identifying the cartridge image.
//
// Returns:
//   Populated database entry. Entry::found is false if the database cannot be
//   loaded or no matching cartridge is found.
// -----------------------------------------------------------------------------
ROMDatabase::Entry
ROMDatabase::LookupBySHA1 (const std::string &database_path, const std::string &sha1)
{
	Entry result;

	if (database_path.empty() || sha1.empty()) {
		return result;
	}

	const std::string normalized_sha1 = normalize_sha1(sha1);

	xmlDoc *const document = xmlReadFile(database_path.c_str(), nullptr, XML_PARSE_NONET);

	if (document == nullptr) {
		return result;
	}

	xmlNodePtr root = xmlDocGetRootElement(document);

	if (root == nullptr || root->type != XML_ELEMENT_NODE || xmlStrcmp(root->name,
		reinterpret_cast<const xmlChar *>("database")) != 0) {

		xmlFreeDoc(document);
		return result;
	}

	xmlNodePtr game = nullptr;

	xmlNodePtr cartridge = FindCartridge(root, reinterpret_cast<const xmlChar *>("sha1"),
						   				 reinterpret_cast<const xmlChar *>(normalized_sha1.c_str()),
										 &game);

	if (cartridge == nullptr) {
		xmlFreeDoc(document);
		return result;
	}

	result.found = true;

	if (game != nullptr) {
		result.game_name = GetProperty(game, "name");
	}

	for (xmlNodePtr board = cartridge->children; board != nullptr; board = board->next) {
		if (board->type != XML_ELEMENT_NODE) {
			continue;
		}

		if (xmlStrcmp(board->name, reinterpret_cast<const xmlChar *>("board")) != 0) {
			continue;
		}

		result.board_type = GetProperty(board, "type");

		for (xmlNodePtr node = board->children; node != nullptr; node = node->next) {
			if (node->type != XML_ELEMENT_NODE) {
				continue;
			}

			if (xmlStrcmp(node->name, reinterpret_cast<const xmlChar *>("chip")) != 0) {
				continue;
			}

			const std::string chip_type = GetProperty(node, "type");

			/*
			 * Skip support logic and continue until the actual Konami VRC
			 * mapper chip is found.
			 */
			if (!is_vrc_chip(chip_type)) {
				continue;
			}

			result.chip_type = chip_type;

			for (xmlNodePtr pin = node->children; pin != nullptr; pin = pin->next) {
				if (pin->type != XML_ELEMENT_NODE) {
					continue;
				}

				if (xmlStrcmp(pin->name, reinterpret_cast<const xmlChar *>("pin")) != 0) {
					continue;
				}

				const std::string number =
					GetProperty(
						pin,
						"number");

				const std::string function = GetProperty(pin, "function");

				if (number == "3") {
					result.pin3_function = function;
				} else if (number == "4") {
					result.pin4_function = function;
				}
			}

			/*
			 * We found the mapper chip, so there is no reason to inspect any
			 * additional chips on this board.
			 */
			break;
		}

		break;
	}

	xmlFreeDoc(document);

	return result;
}
