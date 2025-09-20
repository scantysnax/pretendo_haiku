
#include "Splitters.h"


HorizontalSplitter::HorizontalSplitter(int32 x, int32 y, int32 width)
	: BBox(BRect(x, y, width, y+4), 
	"h_splitter", 
	B_FOLLOW_NONE,
	0,
	B_FANCY_BORDER)
{
}


HorizontalSplitter::~HorizontalSplitter()
{
}


VerticalSplitter::VerticalSplitter (int32 x, int32 y, int32 height)
	: BBox(BRect(x, y, x+4, height),
	"v_splitter",
	B_FOLLOW_NONE,
	0,
	B_FANCY_BORDER)
{
}


VerticalSplitter::~VerticalSplitter()
{
}
