
#ifndef PRETENDO_20120301_H_
#define PRETENDO_20120301_H_

#include <QMainWindow>
#include <chrono>

#include "ui_Pretendo.h"

class QAudioOutput;
class FilesystemModel;
class QLabel;
class QSortFilterProxyModel;
class QTimer;
class Preferences;
class Audio;

#if defined(ENABLE_SOUND)
class Audio;
#else
class NullAudio;
#endif

class Pretendo final : public QMainWindow {
	Q_OBJECT
	friend class Preferences;
	friend class AudioViewer;

public:
	Pretendo(const QString &filename = QString(), QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
	~Pretendo() override;

public:
	void setFrameRate(int framerate);
	void setFrameLimit(uint64_t limit);

private Q_SLOTS:
	void picked(const QModelIndex &index);
	void update();
	void on_action_Load_ROM_triggered();
	void on_action_Free_ROM_triggered();
	void on_action_Run_triggered();
	void on_action_Stop_triggered();
	void on_action_Pause_triggered();
	void on_actionShow_Sprites_toggled(bool value);
	void on_action_Hard_Reset_triggered();
	void on_actionReset_triggered();
	void on_action1x_triggered();
	void on_action2x_triggered();
	void on_action3x_triggered();
	void on_action4x_triggered();
	void on_action_Preferences_triggered();
	void on_actionAbout_Qt_triggered();
	void on_action_About_triggered();
	void on_action_Audio_Viewer_triggered();
	void on_action_Take_Screenshot_triggered();
	void on_action_Pattern_Table_Viewer_triggered();

protected:
	void keyPressEvent(QKeyEvent *event) override;
	void keyReleaseEvent(QKeyEvent *event) override;

private:
	void zoom(int scale);

private:
	Ui::Pretendo ui_;
	Preferences *preferences_            = nullptr;
	FilesystemModel *filesystem_model_   = nullptr;
	QSortFilterProxyModel *filter_model_ = nullptr;
	QTimer *timer_                       = nullptr;
	QLabel *fps_label_                   = nullptr;
	Qt::Key player_1_[8];
	uint64_t framecount_     = 0;
	uint64_t raw_framecount_ = 0;
	bool paused_             = false;
	int framerate_           = 60;

#if defined(ENABLE_SOUND)
	Audio *audio_;
#else
	NullAudio *audio_;
#endif

	uint64_t frame_limit_ = 0;

	std::chrono::time_point<std::chrono::high_resolution_clock> elapsed_time_;
};

#endif
