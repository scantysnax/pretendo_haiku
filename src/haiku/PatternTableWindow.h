#ifndef _PATTERNTABLE_WINDOW_H_
#define _PATTERNTABLE_WINDOW_H_

#include <Window.h>
#include "Settings.h"

class PretendoWindow;
class PatternTableView;    
class CHRExplorerView;


class PatternTableWindow : public BWindow
{
	public:
    		PatternTableWindow(PretendoWindow* parent, int32 which);
    virtual ~PatternTableWindow();

	public:
    virtual bool QuitRequested();
    virtual void Zoom (BPoint origin, float width, float height);
    
    public:
    PatternTableView* View() const 
    { 
    	return fView;
    }
    
	private:
    void LoadSettings();
    void SaveSettings();

	private:
    int32 fWhich = 0;
    PatternTableView *fView = nullptr;
    CHRExplorerView *fExplorer = nullptr;
    PretendoWindow *fParent = nullptr;

	private:
    BMessage *fSettingsMessage = nullptr;
};


#endif // _PATTERNTABLE_WINDOW_H_

