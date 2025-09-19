
#include "VerticalSplitter.h"



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
