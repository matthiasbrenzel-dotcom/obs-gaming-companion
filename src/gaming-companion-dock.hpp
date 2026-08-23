#pragma once

#include <QProcess>
#include <QString>
#include <QWidget>

class QLabel;
class QCheckBox;
class QComboBox;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTimer;

class GamingCompanionDock : public QWidget {
public:
    explicit GamingCompanionDock(QWidget *parent = nullptr);
    ~GamingCompanionDock() override = default;

    void saveShort();
    void addMarker(const QString &type);
    void onReplaySaved(const QString &path);
    int shortDuration() const;

private:
    void buildUi();
    void loadSettings();
    void saveSettings();
    void startReplayBuffer();
    void stopReplayBuffer();
    void browseOutputFolder();
    void browseFfmpeg();
    void browseLogo();
    void browseWhisper();
    void browseWhisperModel();
    void runSubtitlePipeline(const QString &shortFile);
    void runWhisper(const QString &audioFile, const QString &shortFile);
    void burnSubtitles(const QString &shortFile, const QString &srtFile);
    void analyzeHighlightCandidate(const QString &inputFile);
    QString subtitlesOutputPath(const QString &shortFile) const;
    void createClips(const QString &inputFile);
    QString shortOutputPath(const QString &inputFile) const;
    QString clipOutputPath(const QString &inputFile) const;
    QString markerFilePath() const;
    QString sanitizedGameName() const;
    QString detectForegroundProcess() const;
    QString webcamCropFilter() const;
    void refreshDetectedGame();
    void setStatus(const QString &text);

    QLabel *statusLabel_ = nullptr;
    QLabel *replayLabel_ = nullptr;
    QLabel *gameLabel_ = nullptr;
    QComboBox *durationCombo_ = nullptr;
    QComboBox *webcamPositionCombo_ = nullptr;
    QLineEdit *outputFolderEdit_ = nullptr;
    QLineEdit *ffmpegEdit_ = nullptr;
    QLineEdit *logoEdit_ = nullptr;
    QLineEdit *whisperEdit_ = nullptr;
    QLineEdit *whisperModelEdit_ = nullptr;
    QCheckBox *subtitlesCheck_ = nullptr;
    QCheckBox *autoHighlightCheck_ = nullptr;
    QCheckBox *saveOriginalCheck_ = nullptr;
    QCheckBox *logoCheck_ = nullptr;
    QCheckBox *webcamCheck_ = nullptr;
    QSpinBox *webcamWidthSpin_ = nullptr;
    QSpinBox *webcamHeightSpin_ = nullptr;
    QPushButton *saveButton_ = nullptr;
    QProcess ffmpegProcess_;
    QProcess whisperProcess_;
    QProcess subtitleProcess_;
    QProcess highlightProcess_;
    QTimer *gameTimer_ = nullptr;
    int pendingDuration_ = 60;
    QString detectedGame_;
    QString lastShortFile_;
    QString pendingAudioFile_;
    QString pendingSrtBase_;
};
