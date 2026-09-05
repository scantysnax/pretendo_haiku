
#include "ROMInfoView.h"


// -----------------------------------------------------------------------------
// ROMInfoView::ROMInfoView
//
// Creates the ROM Info outline-list view.
//
// Parameters:
//   frame - Initial frame of the view.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
ROMInfoView::ROMInfoView (BRect frame)
	: BOutlineListView (frame, "rom_info_view")
{	
}


// -----------------------------------------------------------------------------
// ROMInfoView::~ROMInfoView
//
// Destroys the ROM Info view.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
ROMInfoView::~ROMInfoView()
{
}


// -----------------------------------------------------------------------------
// ROMInfoView::AttachedToWindow
//
// Completes initialization after the view is attached to its window.  The view
// font and background are configured and the displayed ROM information is
// refreshed from the currently loaded cartridge.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ROMInfoView::AttachedToWindow()
{
	BOutlineListView::AttachedToWindow();

	SetFont(be_fixed_font);
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetFontSize(11.0f);

	Refresh();
}


// -----------------------------------------------------------------------------
// ROMInfoView::Draw
//
// Draws the ROM Info outline-list contents.
//
// Parameters:
//   updateRect - Area of the view that requires redrawing.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ROMInfoView::Draw (BRect updateRect)
{
	BOutlineListView::Draw (updateRect);
}


// -----------------------------------------------------------------------------
// ROMInfoView::Refresh
//
// Rebuilds the ROM information list from the currently loaded cartridge.  If
// no ROM is loaded, the list is cleared and replaced with a simple empty-state
// message.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ROMInfoView::Refresh()
{
	MakeEmpty();


	// -------------------------------------------------------------------------
	// A cartridge must exist before accessing its raw image.
	// -------------------------------------------------------------------------

	if (nes::cart.mapper() == nullptr) {
		AddItem(
			new BStringItem(
				"No ROM loaded."));

		return;
	}


	// -------------------------------------------------------------------------
	// Locate the cartridge database.
	// -------------------------------------------------------------------------

	app_info ai;
	be_app->GetAppInfo(&ai);

	entry_ref ref = ai.ref;

	BPath path(&ref);
	path.GetParent(&path);
	path.Append("nescarts.xml");


	// -------------------------------------------------------------------------
	// Calculate the SHA-1 hash of the currently loaded cartridge image.
	// -------------------------------------------------------------------------

	std::vector<uint8_t> image =
		nes::cart.raw_image();

	hash::sha1 h(
		image.begin(),
		image.end());

	auto digest =
		h.finalize();

	std::string sha1 =
		digest.to_string();

	std::transform(
		sha1.begin(),
		sha1.end(),
		sha1.begin(),
		toupper);


	// -------------------------------------------------------------------------
	// Search the cartridge database for the calculated SHA-1 value.
	// -------------------------------------------------------------------------

	xmlDoc *const file =
		xmlParseFile(path.Path());

	if (file) {
		xmlNodePtr root =
			xmlDocGetRootElement(file);

		if (root) {
			if (xmlStrcmp(
					root->name,
					reinterpret_cast<const xmlChar *>(
						"database")) == 0) {

				rom_match *const rom =
					ProcessDatabase(
						root,
						reinterpret_cast<const xmlChar *>(
							"sha1"),
						reinterpret_cast<const xmlChar *>(
							sha1.c_str()));

				if (rom) {
					DrawROMInfo(rom);
				} else {
					AddItem(
						new BStringItem(
							"ROM not found in cartridge database."));
				}
			}
		}

		xmlFreeDoc(file);
	} else {
		AddItem(
			new BStringItem(
				"Unable to read nescarts.xml."));
	}

	xmlCleanupParser();

	Invalidate();
}


// -----------------------------------------------------------------------------
// ROMInfoView::ProcessDatabase
//
// Searches the ROM database for a game containing a cartridge whose requested
// attribute matches the supplied value.  The matching game and cartridge nodes
// are stored in a static rom_match structure for use by the ROM Info display.
//
// Parameters:
//   root         - Root <database> XML node.
//   search_key   - Cartridge attribute name to compare.
//   search_value - Attribute value to match.
//
// Returns:
//   Pointer to the matching rom_match structure, or nullptr if no match is found.
// -----------------------------------------------------------------------------
ROMInfoView::rom_match_t*
ROMInfoView::ProcessDatabase(xmlNodePtr root, const xmlChar *search_key, const xmlChar *search_value) {

	static rom_match match;

	// get the list of children, this should be text nodes and <game> nodes
	for (xmlNodePtr game = root->children; game; game = game->next) {
		if (xmlStrcmp(game->name, reinterpret_cast<const xmlChar *>("game")) == 0) {
			if (xmlNodePtr node = ProcessGame(game, search_key, search_value)) {
				match.game = game;
				match.cart = node;
				return &match;
			}
		}
	}
	
	return NULL;
}


// -----------------------------------------------------------------------------
// ROMInfoView::DrawROMInfo
//
// Builds the ROM Info outline from a matched database entry.  Game, cartridge,
// peripheral, PRG, CHR, WRAM, mapper/chip, and CIC properties are grouped beneath
// their corresponding outline headings.
//
// Parameters:
//   rom - Matched game/cartridge database entry whose information should be shown.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ROMInfoView::DrawROMInfo(rom_match_t *rom)
{
	BList *list = new BList;
	BString s;
	int32 i;
	
	BListItem *gameInfoItem = new BStringItem("Game Info");
	AddItem(gameInfoItem);
	
	BListItem *cartInfoItem = new BStringItem("Cart Info");
	AddItem(cartInfoItem);
	
	BListItem *peripheralItem = new BStringItem("Peripherals");
	AddItem(peripheralItem);
	
	BListItem *prgItem = new BStringItem("PRG Info");
	AddItem(prgItem);
	
	BListItem *chrItem = new BStringItem("CHR Info");
	AddItem(chrItem); 
	
	BListItem *wramItem = new BStringItem("WRAM Info");
	AddItem(wramItem);	
	
	BListItem *mapperItem = new BStringItem("Mapper Info");
	AddItem(mapperItem);
	
	BListItem *cicItem = new BStringItem("CIC (Lockout Chip) Info");
	AddItem(cicItem);
	
	for (xmlAttr *properties = rom->game->properties; properties; properties = properties->next) {
		s.SetToFormat("%-15s: %s", properties->name, xmlGetProp(rom->game, properties->name));
		list->AddItem(new BStringItem(s.String()));
	}
	
	for (i = list->CountItems()-1; i >= 0; i--) {
		BStringItem *item = reinterpret_cast<BStringItem *>(list->ItemAt(i));
		AddUnder(item, gameInfoItem);
	}
	
	list->MakeEmpty();
	
	for (xmlAttr *properties = rom->cart->properties; properties; properties = properties->next) {
		s.SetToFormat("%-15s : %s", properties->name, xmlGetProp(rom->cart, properties->name));
		list->AddItem(new BStringItem(s.String()));
	}
	
	for (i = list->CountItems()-1; i >= 0; i--) {
		BStringItem *item = reinterpret_cast<BStringItem *>(list->ItemAt(i));
		AddUnder(item, cartInfoItem);
	}
	
	list->MakeEmpty();
	
	
	
	// get the peripherals
	for (xmlNodePtr node = rom->game->children; node; node = node->next) {		
		if (xmlStrcmp(node->name, reinterpret_cast<const xmlChar *>("peripherals")) == 0) {
			for (xmlNodePtr device = node->children; device; device = device->next) {
				for (xmlAttr *properties = device->properties; properties; properties = properties->next) {
					s.SetToFormat("%-15s : %s", properties->name, xmlGetProp(device, properties->name));
					list->AddItem(new BStringItem(s.String()));
				}
			}
			
			for (i = list->CountItems()-1; i >= 0; i--) {
				BStringItem *item = reinterpret_cast<BStringItem *>(list->ItemAt(i));
				AddUnder(item, peripheralItem);
			}
		}
	}

	list->MakeEmpty();
	
	// get the board info
	for (xmlNodePtr board = rom->cart->children; board; board = board->next) {
		if (xmlStrcmp(board->name, reinterpret_cast<const xmlChar *>("board")) == 0) {
			for (xmlNodePtr node = board->children; node; node = node->next) {
				if (xmlStrcmp(node->name, reinterpret_cast<const xmlChar *>("prg")) == 0) {
					for (xmlAttr *properties = node->properties; properties; properties = properties->next) {
						s.SetToFormat("%-15s : %s", properties->name, xmlGetProp(node, properties->name));
						list->AddItem(new BStringItem(s.String()));
					}
					
					for (i = list->CountItems()-1; i >= 0; i--) {
						BStringItem *item = reinterpret_cast<BStringItem *>(list->ItemAt(i));
						AddUnder(item, prgItem);
					}
					
				}
				
				list->MakeEmpty();
				
				if (xmlStrcmp(node->name, reinterpret_cast<const xmlChar *>("chr")) == 0) {
					for (xmlAttr *properties = node->properties; properties; properties = properties->next) {
						s.SetToFormat("%-15s : %s", properties->name, xmlGetProp(node, properties->name));
						list->AddItem(new BStringItem(s.String()));
					}
					
					for (i = list->CountItems()-1; i >= 0; i--) {
						BStringItem *item = reinterpret_cast<BStringItem *>(list->ItemAt(i));
						AddUnder(item, chrItem);
					}	
				}
				
				list->MakeEmpty();

				if (xmlStrcmp(node->name, reinterpret_cast<const xmlChar *>("wram")) == 0) {
					for (xmlAttr *properties = node->properties; properties; properties = properties->next) {
						s.SetToFormat("%-15s : %s", properties->name, xmlGetProp(node, properties->name));
						list->AddItem(new BStringItem(s.String()));
					}

					for (i = list->CountItems()-1; i >= 0; i--) {
						BStringItem *item = reinterpret_cast<BStringItem *>(list->ItemAt(i));
						AddUnder(item, wramItem);
					}
				}
								
				list->MakeEmpty();
			
				if (xmlStrcmp(node->name, reinterpret_cast<const xmlChar *>("chip")) == 0) {
					for (xmlAttr *properties = node->properties; properties; properties = properties->next) {
						s.SetToFormat("%-15s : %s", properties->name, xmlGetProp(node, properties->name));
						list->AddItem(new BStringItem(s.String()));
					}
					
					for (i = list->CountItems()-1; i >= 0; i--) {
						BStringItem *item = reinterpret_cast<BStringItem *>(list->ItemAt(i));
						AddUnder(item, mapperItem);
					}
				}
				
				list->MakeEmpty();
				
				if (xmlStrcmp(node->name, reinterpret_cast<const xmlChar *>("cic")) == 0) {
					for (xmlAttr *properties = node->properties; properties; properties = properties->next) {
						s.SetToFormat("%-15s : %s", properties->name, xmlGetProp(node, properties->name));
						list->AddItem(new BStringItem(s.String()));
					}
					
					for (i = list->CountItems()-1; i >= 0; i--) {
						BStringItem *item = reinterpret_cast<BStringItem *>(list->ItemAt(i));
						AddUnder(item, cicItem);
					}
				}
				
				list->MakeEmpty();
			}	
		}
	}
}



// -----------------------------------------------------------------------------
// ROMInfoView::ProcessGame
//
// Searches the cartridge entries beneath a game node for an attribute matching
// the requested key/value pair.  When a matching cartridge is found, its Cart
// ID is added to the view and the matching cartridge node is returned.
//
// Parameters:
//   game         - XML <game> node whose cartridge entries should be searched.
//   search_key   - Cartridge attribute name to compare.
//   search_value - Attribute value to match.
//
// Returns:
//   Pointer to the matching <cartridge> node, or nullptr if no match is found.
// -----------------------------------------------------------------------------
xmlNodePtr 
ROMInfoView::ProcessGame (xmlNodePtr game, const xmlChar *search_key, const xmlChar *search_value) 
{
	// get the list of children, this should be text nodes and <cartridge> nodes
	for(xmlNodePtr cartridge = game->children; cartridge; cartridge = cartridge->next) {
		if (xmlStrcmp(cartridge->name, reinterpret_cast<const xmlChar *>("cartridge")) == 0) {
			// ok we are looking at a cart, let's look at the attributes

			if(xmlChar *const value = xmlGetProp(cartridge, search_key)) {
				if (xmlStrcmp(value, search_value) == 0) {
					BString buffer;
					
					buffer << "Cart ID: " << reinterpret_cast<char *>(value) << 
								" " << reinterpret_cast<const char *>(search_value);
					AddItem(new BStringItem(buffer));
					
					return cartridge;
				}
			}
		}
	}
	
	return nullptr;
}

