#ifndef _APU_STATUS_VIEW_H_
#define _APU_STATUS_VIEW_H_

#include <View.h>


class APUStatusView : public BView {
	public:
	APUStatusView(BRect frame);

	public:
	virtual void AttachedToWindow();
	virtual void Draw(BRect updateRect);
	virtual void Pulse();

	private:
	void DrawSectionHeader(const char *text, float x, float y);
	void DrawTextLine(const char *label, const char *value, float x, float y);
	void DrawBoolLine(const char *label, bool value, float x, float y);
};


#endif // _APU_STATUS_VIEW_H_
