#ifndef _MAPPER_EXPLORER_WINDOW_H_
#define _MAPPER_EXPLORER_WINDOW_H_

#include <Window.h>

#include "MapperExplorerView.h"


class PretendoWindow;


// -----------------------------------------------------------------------------
// MapperExplorerWindow
//
// Floating debugger window that owns a MapperExplorerView.
// -----------------------------------------------------------------------------
class MapperExplorerWindow : public BWindow
{
	public:
			MapperExplorerWindow (PretendoWindow *parent);
	virtual ~MapperExplorerWindow();

	public:
	virtual bool QuitRequested();

	private:
	PretendoWindow *fParent = nullptr;
	MapperExplorerView *fView = nullptr;
};


#endif // _MAPPER_EXPLORER_WINDOW_H_
