#ifndef DEBUG_HELPERS_H_
#define DEBUG_HELPERS_H_

#include <View.h>

static inline void
StrokeRectTriple(BView* v, BRect r, rgb_color mid)
{
	if (!v)
		return;

	v->SetHighColor(0, 0, 0, 255);
	v->StrokeRect(r.InsetByCopy(-2, -2));

	v->SetHighColor(mid);
	v->StrokeRect(r.InsetByCopy(-1, -1));

	v->SetHighColor(255, 255, 255, 255);
	v->StrokeRect(r);
}

static inline void
FillAndStrokeRectTriple(BView* v, BRect r, rgb_color fill, rgb_color stroke)
{
	if (!v)
		return;

	v->PushState();

	v->SetDrawingMode(B_OP_ALPHA);
	v->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	v->SetHighColor(fill);
	v->FillRect(r);

	v->SetDrawingMode(B_OP_COPY);
	StrokeRectTriple(v, r, stroke);

	v->PopState();
}

#endif
