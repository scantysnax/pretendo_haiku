
#ifndef _NAMETABLE_WINDOW_H_
#define _NAMETABLE_WINDOW_H_

#include <Window.h>

#include "NameTableView.h"
#include "Settings.h"


class PretendoWindow;
class NameTableView;
class PatternTableWindow;
class CHRExplorerView;


class NameTableWindow : public BWindow
{
	public:
	typedef enum {
		HEIGHT = 30*8,
		WIDTH = 32*8
	} nametable_size;
		
	
	public:
			NameTableWindow(PretendoWindow *parent, int32 which,
							PatternTableWindow* pt0, PatternTableWindow* pt1);
	virtual ~NameTableWindow();
	
	public:
	virtual bool QuitRequested();
	virtual void MessageReceived (BMessage *message);
	
	 public:
	 NameTableView *View() const
	 { 
	 	return fView;
	 }
	 
	 public:
	 void SetPatternTables(PatternTableWindow *pt0, PatternTableWindow *pt1);
	  
	private:
	void LoadSettings();
	void SaveSettings();
	
	private:
	int32 fWhich = 0;
	PretendoWindow *fParent = nullptr;
	
	private:
	NameTableView *fView = nullptr;
	CHRExplorerView *fExplorer = nullptr;
	
	private:
	BMessage *fSettingsMessage;
};

#endif // _NAMETABLE_WINDOW_H_
