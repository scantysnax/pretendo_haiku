
#include "ROMInfoView.h"


using nes::cart;


ROMInfoView::ROMInfoView (BRect frame)
	: BOutlineListView (frame, "rom_info_view")
{	
}


ROMInfoView::~ROMInfoView()
{
}


void
ROMInfoView::AttachedToWindow()
{	
	app_info ai;
	be_app->GetAppInfo(&ai);
	entry_ref ref = ai.ref;
	BPath path(&ref);
	path.GetParent(&path);
	path.Append("nescarts.xml");

	
	std::vector<uint8_t> image = nes::cart.raw_image();
	hash::sha1 h(image.begin(), image.end());
	auto digest = h.finalize();
	std::string sha1 = digest.to_string();
    std::transform(sha1.begin(), sha1.end(), sha1.begin(), toupper);

	xmlDoc *const file = xmlParseFile(path.Path());
	
    if (file) {
        // get the root element it should be <database>
		if(const xmlNodePtr root = xmlDocGetRootElement(file)) {
			if (xmlStrcmp(root->name, 
				reinterpret_cast<const xmlChar *>("database")) == 0) {
				if (rom_match *const rom = ProcessDatabase(root, 
					reinterpret_cast<const xmlChar *>("sha1"), 
					reinterpret_cast<const xmlChar *>(sha1.c_str()))) {
					// goto work!
					DrawROMInfo(rom);
				} else {
					(new BAlert(0, "Couldn't find a match.", "Sorry"))->Go();
				}
			}
		}
    } else {
    	(new BAlert("Error", "Can't parse file 'nescarts.xml'", "Oops"))->Go();
    }
		
	xmlFreeDoc(file);
	xmlCleanupParser();	
	
	SetFont(be_fixed_font);
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetFontSize(11.0f);
	
	BOutlineListView::AttachedToWindow();
}


void
ROMInfoView::Draw (BRect updateRect)
{
	BOutlineListView::Draw (updateRect);
}


//------------------------------------------------------------------------------
// Name: process_game
// Desc: iterates the <document> childrent of a <cartridge> node.
// Returns: NULL or a pointer to a the <cartridge> node which had a matching 
//          property/value pair
//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
// Name: process_database
// Desc: iterates the <game> childrent of a <document> node.
// Returns: NULL or a pointer to a rom_match object (statically allocated, 
//          no need to free it
//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------
// Name: print_info
// Desc: prints the info associated with a given game/cart
//------------------------------------------------------------------------------
void
ROMInfoView::DrawROMInfo(rom_match_t *rom)
{
	BList *list = new BList;
	//char buffer[1024];
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
						list->AddItem(new BStringItem(s.String());
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
