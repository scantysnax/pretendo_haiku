
#ifndef	_ROM_FILE_PANEL_H_
#define _ROM_FILE_PANEL_H_

#include <Entry.h>
#include <FilePanel.h>
#include <Path.h>
#include <String.h>
#include <Window.h>


class ROMFilePanel : public BFilePanel
{
	public:
			ROMFilePanel();
	virtual ~ROMFilePanel();
	
	private:
	virtual void SelectionChanged (void);
	
	private:
	void Customize (void);
	
	private:
	entry_ref fPrevRef;
};


class ROMFilter : public BRefFilter
{
	public:
	virtual bool Filter (const entry_ref *ref, BNode *node, struct  stat_beos *st, 
		const char *filetype)
	{ 
		(void)node;
		(void)st;
		
		BString	fileName (ref->name);
		BString fileType (filetype);
		int32 pos;
	
		// first check the file type.
		// we don't want to filter out directories, symlinks, or volumes.
		if (fileType.ICompare ("application/x-vnd.Be-directory") 	== 0 ||
			fileType.ICompare ("application/x-vnd.Be-volume") 		== 0 ||
			fileType.ICompare ("application/x-vnd.Be.symlink")		== 0) {
			return true;
		}
			
		// otherwise, we'll go ahead, analyse the file's extension 
		// and determine what to do from there.
	
		pos = fileName.FindLast ('.');
		if (pos == B_ERROR) {
			return false;
		}
	
		fileName.Remove (0, ++pos);
		if (fileName.ICompare ("nes") 	== 0 ||	// iNES format
			fileName.ICompare ("unf") 	== 0 ||	// UNIF archive (DOS)
			fileName.ICompare ("unif") 	== 0 ||	// UNIF archive (UNIX)
			fileName.ICompare ("fds") 	== 0) {	// FDS format
			return true;
		}
	
		// we couldn't catch anything.
		return false;
	}
};

#endif //	_ROM_FILE_PANEL_H_


