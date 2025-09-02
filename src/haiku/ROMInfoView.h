
#ifndef _ROM_INFO_VIEW_
#define _ROM_INFO_VIEW_

#include <OutlineListView.h>
#include <libxml2/libxml/parser.h>
#include "sha1.h"


class ROMInfoView : public BOutlineListView
{
	public:
	ROMInfoView(BRect frame);
	~ROMInfoView();
	
	public:
	typedef struct rom_match {
		xmlNodePtr game;
		xmlNodePtr cart;
	} rom_match_t;
	
	public:
	virtual void AttachedToWindow (void);
	virtual void Draw (BRect frame);
	
	private:
	void PrintInfo(rom_match *rom);
	rom_match_t *ProcessDatabase(xmlNodePtr root, const xmlChar *search_key, const xmlChar 									*search_value);
	xmlNodePtr ProcessGame(xmlNodePtr game, const xmlChar *search_key, 
							const xmlChar *search_value);
};
	

#endif // _ROM_INFO_VIEW
