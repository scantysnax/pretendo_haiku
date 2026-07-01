#ifndef _DEBUG_HELPERS_H_
#define _DEBUG_HELPERS_H_

#include <Point.h>
#include <Rect.h>
#include <View.h>

static inline void
StrokeRectTriple (BView *v, BRect r, rgb_color mid)
{
	if (!v) {
		return;
	}

	v->SetHighColor(0, 0, 0, 255);
	v->StrokeRect(r.InsetByCopy(-2, -2));

	v->SetHighColor(mid);
	v->StrokeRect(r.InsetByCopy(-1, -1));

	v->SetHighColor(255, 255, 255, 255);
	v->StrokeRect(r);
}


static inline void
FillAndStrokeRectTriple (BView *v, BRect r, rgb_color fill, rgb_color stroke)
{
	if (!v) {
		return;
	}

	v->PushState();

	v->SetDrawingMode(B_OP_ALPHA);
	v->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	v->SetHighColor(fill);
	v->FillRect(r);

	v->SetDrawingMode(B_OP_COPY);
	StrokeRectTriple(v, r, stroke);

	v->PopState();
}


static inline void
DrawDebugPanel (BView *view, BRect rect, const char *title)
{
	if (!view) {
		return;
	}

	view->SetHighColor(228, 228, 228, 255);
	view->FillRect(rect);

	view->SetHighColor(170, 170, 170, 255);
	view->StrokeRect(rect);

	view->SetFontSize(11.0f);

	const float x = rect.left + 6.0f;
	const float titleY = rect.top + 13.0f;
	const float dividerY = rect.top + 20.0f;

	view->SetHighColor(70, 70, 70, 255);
	view->DrawString(title, BPoint(x, titleY));

	view->SetHighColor(120, 120, 120, 255);
	view->StrokeLine(
		BPoint(rect.left + 6.0f, dividerY),
		BPoint(rect.right - 6.0f, dividerY)
	);
}

#endif
