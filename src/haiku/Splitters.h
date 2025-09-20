
#ifndef _SPLITTERS_H_
#define _SPLITTERS_H_

#include <Box.h>


class HorizontalSplitter : public BBox
{
	public:
			HorizontalSplitter(int32 x, int32 y, int32 width);
	virtual ~HorizontalSplitter();
	
	private:
	
};


class VerticalSplitter : public BBox
{
	public:
			VerticalSplitter(int32 x, int32 y, int32 height);
	virtual ~VerticalSplitter();
	
	private:	
};


#endif // _SPLITTERS_H_
