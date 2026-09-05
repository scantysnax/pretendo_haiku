
#ifndef _APU_STATUS_VIEW_H_
#define _APU_STATUS_VIEW_H_

#include <CheckBox.h>
#include <Font.h>
#include <Message.h>
#include <String.h>
#include <View.h>
#include <Window.h>

#include "Apu.h"
#include "Cart.h"
#include "PretendoWindow.h"


constexpr double kNESClockRate = 1789773.0;

constexpr float kTopMargin = 14.0f;

constexpr float kColumn1X = 16.0f;
constexpr float kColumn2X = 306.0f;
constexpr float kColumn3X = 576.0f;

constexpr float kAPUPanelWidth = 	270.0f;
constexpr float kPanelWidth = 		250.0f;

constexpr float kLineHeight = 16.0f;
constexpr float kSectionGap = 14.0f;

constexpr float kPanelPadding = 10.0f;
constexpr float kHeaderHeight = 26.0f;

constexpr bigtime_t kPulseRate = 100000;

const rgb_color kPanelColor = 		{ 248, 248, 248, 255 };
const rgb_color kPanelBorderColor = { 170, 175, 180, 255 };

const rgb_color kHeaderColor = 		{ 190, 190, 190, 255 };
const rgb_color kHeaderTextColor = 	{ 35, 35, 35, 255 };

const rgb_color kLabelColor = { 60, 60, 60, 255 };
const rgb_color kValueColor = { 15, 15, 15, 255 };

const rgb_color kEnabledColor = 	{ 30, 110, 45, 255 };
const rgb_color kDisabledColor =	{ 120, 120, 120, 255 };

const rgb_color kMutedColor = 	{ 150, 70, 30, 255 };
const rgb_color kIRQColor = 	{ 170, 35, 35, 255 };


class APUStatusView : public BView
{
	public:
			APUStatusView (BRect frame);
	virtual	~APUStatusView();
	
	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void Pulse();
	
	private:
	float PanelHeightForLines (int32 lines);
	
	private:
	double SquareFrequencyHz (uint16 timerPeriod);
	double TriangleFrequencyHz (uint16 timerPeriod);
	double NoiseClockRateHz (uint16 timerPeriod);
	double DMCBitRateHz (uint16 timerPeriod);
	
	private:
	const char *SquareStateText (bool enabled, bool muted, uint8 lengthCounter, 
								 bool sweepSilenced, uint16 timerFrequency, uint8 output);
	const char *TriangleStateText (bool enabled, bool muted, uint8 lengthCounter, uint8 linearCounter);
	const char *NoiseStateText (bool enabled, bool muted, uint8 lengthCounter, uint8 envelopeVolume, uint8 output);
	const char *DMCStateText (bool active, bool muted, bool sampleBufferEmpty, uint16 bytesRemaining);
	
	private:
	float PanelContentY (BRect panel);
	void DrawPanelBackground(BRect rect, const char *title);
	void DrawSectionHeader(const char *text, float x, float y);
	void DrawTextLine(const char *label, const char *value, float x, float y);
	void DrawBoolLine(const char *label, bool value, float x, float y);
	void DrawFixedTextLine(const char *label, const char *value, float x, float y);
	
	private:
	void DrawAPUPanel (BRect panel);
	void DrawSquare1Panel (BRect panel);
	void DrawSquare2Panel (BRect panel);
	void DrawTrianglePanel (BRect panel);
	void DrawNoisePanel (BRect panel);
	void DrawDMCPanel (BRect panel);
	
	private:
	void SetChannelControlsVisible(bool visible);
	bool HasROMLoaded() const;
	void DrawNoROMMessage(BRect panel);
	
	private:
	bool fChannelControlsVisible = true;
	
	private:
	BCheckBox *fSquare1CheckBox = nullptr;
	BCheckBox *fSquare2CheckBox = nullptr;
	BCheckBox *fTriangleCheckBox = nullptr;
	BCheckBox *fNoiseCheckBox = nullptr;
	BCheckBox *fDMCCheckBox = nullptr;
};


#endif // _APU_STATUS_VIEW_H_
