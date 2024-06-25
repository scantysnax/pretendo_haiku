
#ifndef _CART_INFO_VIEW_
#define _CART_INFO_VIEW_

#include <OutlineListView.h>
#include <String.h>
#include <Alert.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <boost/uuid/detail/sha1.hpp>
#include <libxml2/libxml/parser.h>

#include "sha1.h"

#include <stdio.h>


class CartInfoView : public BOutlineListView
{
	public:
	typedef struct rom_match {
		xmlNodePtr game;
		xmlNodePtr cart;
	} rom_match_t;
	
	public:
	CartInfoView(BRect frame);
	~CartInfoView();
	
	public:
	virtual void AttachedToWindow (void);
	virtual void Draw (BRect frame);
	
	private:
	void PrintInfo(rom_match *rom);
	rom_match_t *ProcessDatabase(xmlNodePtr root, const xmlChar *search_key, const xmlChar 									*search_value);
	xmlNodePtr ProcessGame(xmlNodePtr game, const xmlChar *search_key, 
							const xmlChar *search_value);
};
	

#endif // _CART_INFO_VIEW
