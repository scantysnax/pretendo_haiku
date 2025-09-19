
#include "HorizontalSplitter.h"


HorizontalSplitter::HorizontalSplitter(int32 x, int32 y, int32 width)
	: BBox(BRect(x, y, width, y+4), "hsplitter")
{
}


HorizontalSplitter::~HorizontalSplitter()
{
}

