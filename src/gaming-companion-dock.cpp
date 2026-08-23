#include "gaming-companion-dock.hpp"

#include <obs-frontend-api.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QDate>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#endif

namespace {
constexpr auto kOrg = "Gefechtszone";
constexpr auto kApp = "OBSGamingCompanion";
}

GamingCompanionDock::GamingCompanionDock(QWidget *parent) : QWidget(parent)
{
    buildUi();
    loadSettings();

    connect(&ffmpegProcess_, &QProcess::started, this, [this]() {
        setStatus(QStringLiteral("16:9-Clip und Short werden erstellt …"));
    });
    connect(&ffmpegProcess_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus status) {
                if (status == QProcess::NormalExit && code == 0) {
                    if (subtitlesCheck_->isChecked())
                        runSubtitlePipeline(lastShortFile_);
                    else
                        setStatus(QStringLiteral("Fertig: 16:9-Clip / 9:16-Short gespeichert."));
                } else {
                    setStatus(QStringLiteral("FFmpeg-Fehler. Prüfe FFmpeg-Pfad und Einstellungen."));
                }
            });

    connect(&whisperProcess_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus status) {
                const QString srt = pendingSrtBase_ + QStringLiteral(".srt");
                if (status == QProcess::NormalExit && code == 0 && QFileInfo::exists(srt))
                    burnSubtitles(lastShortFile_, srt);
                else
                    setStatus(QStringLiteral("Short gespeichert, aber Whisper konnte keine Untertitel erzeugen."));
            });

    connect(&subtitleProcess_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus status) {
                if (status == QProcess::NormalExit && code == 0)
                    setStatus(QStringLiteral("Fertig: Short mit lokalen automatischen Untertiteln gespeichert."));
                else
                    setStatus(QStringLiteral("Untertitel wurden erkannt, konnten aber nicht eingebrannt werden."));
            });

    gameTimer_ = new QTimer(this);
    gameTimer_->setInterval(2000);
    connect(gameTimer_, &QTimer::timeout, this, [this]() { refreshDetectedGame(); });
    gameTimer_->start();
    refreshDetectedGame();
}

void GamingCompanionDock::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);

    auto *title = new QLabel(QStringLiteral("<b>OBS Gaming Companion 0.5.1</b>"), this);
    root->addWidget(title);

    gameLabel_ = new QLabel(QStringLiteral("Spiel: wird erkannt …"), this);
    gameLabel_->setWordWrap(true);
    root->addWidget(gameLabel_);

    replayLabel_ = new QLabel(this);
    replayLabel_->setWordWrap(true);
    root->addWidget(replayLabel_);

    auto *replayButtons = new QHBoxLayout();
    auto *startButton = new QPushButton(QStringLiteral("Replay starten"), this);
    auto *stopButton = new QPushButton(QStringLiteral("Stoppen"), this);
    replayButtons->addWidget(startButton);
    replayButtons->addWidget(stopButton);
    root->addLayout(replayButtons);
    connect(startButton, &QPushButton::clicked, this, [this]() { startReplayBuffer(); });
    connect(stopButton, &QPushButton::clicked, this, [this]() { stopReplayBuffer(); });

    auto *form = new QFormLayout();
    durationCombo_ = new QComboBox(this);
    durationCombo_->addItem(QStringLiteral("30 Sekunden"), 30);
    durationCombo_->addItem(QStringLiteral("60 Sekunden"), 60);
    durationCombo_->addItem(QStringLiteral("90 Sekunden"), 90);
    durationCombo_->setCurrentIndex(1);
    form->addRow(QStringLiteral("Clip-Länge:"), durationCombo_);

    saveOriginalCheck_ = new QCheckBox(QStringLiteral("16:9 zusätzlich speichern"), this);
    saveOriginalCheck_->setChecked(true);
    form->addRow(QStringLiteral("Parallel-Clip:"), saveOriginalCheck_);

    auto *folderRow = new QWidget(this);
    auto *folderLayout = new QHBoxLayout(folderRow);
    folderLayout->setContentsMargins(0, 0, 0, 0);
    outputFolderEdit_ = new QLineEdit(QStandardPaths::writableLocation(QStandardPaths::MoviesLocation) + "/OBS Gaming Companion", folderRow);
    auto *folderButton = new QPushButton(QStringLiteral("…"), folderRow);
    folderLayout->addWidget(outputFolderEdit_);
    folderLayout->addWidget(folderButton);
    form->addRow(QStringLiteral("Ausgabeordner:"), folderRow);
    connect(folderButton, &QPushButton::clicked, this, [this]() { browseOutputFolder(); });

    auto *ffmpegRow = new QWidget(this);
    auto *ffmpegLayout = new QHBoxLayout(ffmpegRow);
    ffmpegLayout->setContentsMargins(0, 0, 0, 0);
#ifdef Q_OS_WIN
    ffmpegEdit_ = new QLineEdit(QStringLiteral("ffmpeg.exe"), ffmpegRow);
#else
    ffmpegEdit_ = new QLineEdit(QStringLiteral("ffmpeg"), ffmpegRow);
#endif
    auto *ffmpegButton = new QPushButton(QStringLiteral("…"), ffmpegRow);
    ffmpegLayout->addWidget(ffmpegEdit_);
    ffmpegLayout->addWidget(ffmpegButton);
    form->addRow(QStringLiteral("FFmpeg:"), ffmpegRow);
    connect(ffmpegButton, &QPushButton::clicked, this, [this]() { browseFfmpeg(); });

    subtitlesCheck_ = new QCheckBox(QStringLiteral("Automatische Untertitel (lokal mit whisper.cpp)"), this);
    form->addRow(QStringLiteral("Untertitel:"), subtitlesCheck_);

    auto *whisperRow = new QWidget(this);
    auto *whisperLayout = new QHBoxLayout(whisperRow);
    whisperLayout->setContentsMargins(0, 0, 0, 0);
#ifdef Q_OS_WIN
    whisperEdit_ = new QLineEdit(QStringLiteral("whisper-cli.exe"), whisperRow);
#else
    whisperEdit_ = new QLineEdit(QStringLiteral("whisper-cli"), whisperRow);
#endif
    auto *whisperButton = new QPushButton(QStringLiteral("…"), whisperRow);
    whisperLayout->addWidget(whisperEdit_);
    whisperLayout->addWidget(whisperButton);
    form->addRow(QStringLiteral("Whisper CLI:"), whisperRow);
    connect(whisperButton, &QPushButton::clicked, this, [this]() { browseWhisper(); });

    auto *modelRow = new QWidget(this);
    auto *modelLayout = new QHBoxLayout(modelRow);
    modelLayout->setContentsMargins(0, 0, 0, 0);
    whisperModelEdit_ = new QLineEdit(modelRow);
    whisperModelEdit_->setPlaceholderText(QStringLiteral("z. B. ggml-small.bin"));
    auto *modelButton = new QPushButton(QStringLiteral("…"), modelRow);
    modelLayout->addWidget(whisperModelEdit_);
    modelLayout->addWidget(modelButton);
    form->addRow(QStringLiteral("Whisper-Modell:"), modelRow);
    connect(modelButton, &QPushButton::clicked, this, [this]() { browseWhisperModel(); });

    autoHighlightCheck_ = new QCheckBox(QStringLiteral("Automatische Highlight-Kandidaten aus Audio-Peaks protokollieren"), this);
    form->addRow(QStringLiteral("Auto-Highlight:"), autoHighlightCheck_);

    logoCheck_ = new QCheckBox(QStringLiteral("Logo im Short einblenden"), this);
    form->addRow(QStringLiteral("Logo:"), logoCheck_);

    auto *logoRow = new QWidget(this);
    auto *logoLayout = new QHBoxLayout(logoRow);
    logoLayout->setContentsMargins(0, 0, 0, 0);
    logoEdit_ = new QLineEdit(logoRow);
    logoEdit_->setPlaceholderText(QStringLiteral("PNG mit Transparenz"));
    auto *logoButton = new QPushButton(QStringLiteral("…"), logoRow);
    logoLayout->addWidget(logoEdit_);
    logoLayout->addWidget(logoButton);
    form->addRow(QStringLiteral("Logo-Datei:"), logoRow);
    connect(logoButton, &QPushButton::clicked, this, [this]() { browseLogo(); });

    webcamCheck_ = new QCheckBox(QStringLiteral("Webcam aus 16:9-Bild hervorheben"), this);
    form->addRow(QStringLiteral("Webcam:"), webcamCheck_);

    webcamPositionCombo_ = new QComboBox(this);
    webcamPositionCombo_->addItem(QStringLiteral("Oben links"), "tl");
    webcamPositionCombo_->addItem(QStringLiteral("Oben rechts"), "tr");
    webcamPositionCombo_->addItem(QStringLiteral("Unten links"), "bl");
    webcamPositionCombo_->addItem(QStringLiteral("Unten rechts"), "br");
    webcamPositionCombo_->setCurrentIndex(3);
    form->addRow(QStringLiteral("Webcam im 16:9-Bild:"), webcamPositionCombo_);

    auto *webcamSize = new QWidget(this);
    auto *webcamSizeLayout = new QHBoxLayout(webcamSize);
    webcamSizeLayout->setContentsMargins(0, 0, 0, 0);
    webcamWidthSpin_ = new QSpinBox(webcamSize);
    webcamWidthSpin_->setRange(160, 960);
    webcamWidthSpin_->setValue(420);
    webcamHeightSpin_ = new QSpinBox(webcamSize);
    webcamHeightSpin_->setRange(90, 540);
    webcamHeightSpin_->setValue(236);
    webcamSizeLayout->addWidget(new QLabel(QStringLiteral("B"), webcamSize));
    webcamSizeLayout->addWidget(webcamWidthSpin_);
    webcamSizeLayout->addWidget(new QLabel(QStringLiteral("H"), webcamSize));
    webcamSizeLayout->addWidget(webcamHeightSpin_);
    form->addRow(QStringLiteral("Webcam-Ausschnitt:"), webcamSize);

    root->addLayout(form);

    saveButton_ = new QPushButton(QStringLiteral("SHORT + CLIP speichern"), this);
    saveButton_->setMinimumHeight(42);
    root->addWidget(saveButton_);
    connect(saveButton_, &QPushButton::clicked, this, [this]() { saveShort(); });

    auto *markerTitle = new QLabel(QStringLiteral("<b>Highlight-Marker</b>"), this);
    root->addWidget(markerTitle);
    auto *markerRow = new QHBoxLayout();
    const QStringList markerNames{QStringLiteral("Kill"), QStringLiteral("Highlight"), QStringLiteral("Lustig"), QStringLiteral("Bug")};
    for (const QString &name : markerNames) {
        auto *button = new QPushButton(name, this);
        markerRow->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, name]() { addMarker(name); });
    }
    root->addLayout(markerRow);

    statusLabel_ = new QLabel(QStringLiteral("Bereit."), this);
    statusLabel_->setWordWrap(true);
    root->addWidget(statusLabel_);
    root->addStretch(1);

    replayLabel_->setText(obs_frontend_replay_buffer_active()
                              ? QStringLiteral("Replay-Puffer: aktiv")
                              : QStringLiteral("Replay-Puffer: aus"));

    const auto saveOnChange = [this]() { saveSettings(); };
    connect(durationCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, saveOnChange);
    connect(saveOriginalCheck_, &QCheckBox::toggled, this, saveOnChange);
    connect(logoCheck_, &QCheckBox::toggled, this, saveOnChange);
    connect(webcamCheck_, &QCheckBox::toggled, this, saveOnChange);
    connect(webcamPositionCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, saveOnChange);
    connect(webcamWidthSpin_, qOverload<int>(&QSpinBox::valueChanged), this, saveOnChange);
    connect(webcamHeightSpin_, qOverload<int>(&QSpinBox::valueChanged), this, saveOnChange);
    connect(outputFolderEdit_, &QLineEdit::editingFinished, this, saveOnChange);
    connect(ffmpegEdit_, &QLineEdit::editingFinished, this, saveOnChange);
    connect(logoEdit_, &QLineEdit::editingFinished, this, saveOnChange);
    connect(subtitlesCheck_, &QCheckBox::toggled, this, saveOnChange);
    connect(autoHighlightCheck_, &QCheckBox::toggled, this, saveOnChange);
    connect(whisperEdit_, &QLineEdit::editingFinished, this, saveOnChange);
    connect(whisperModelEdit_, &QLineEdit::editingFinished, this, saveOnChange);
}

void GamingCompanionDock::loadSettings()
{
    QSettings s(kOrg, kApp);
    const int duration = s.value("duration", 60).toInt();
    const int idx = durationCombo_->findData(duration);
    if (idx >= 0)
        durationCombo_->setCurrentIndex(idx);
    outputFolderEdit_->setText(s.value("outputFolder", outputFolderEdit_->text()).toString());
    ffmpegEdit_->setText(s.value("ffmpeg", ffmpegEdit_->text()).toString());
    logoEdit_->setText(s.value("logoPath", QString()).toString());
    logoCheck_->setChecked(s.value("logoEnabled", false).toBool());
    saveOriginalCheck_->setChecked(s.value("saveOriginal", true).toBool());
    webcamCheck_->setChecked(s.value("webcamEnabled", false).toBool());
    webcamWidthSpin_->setValue(s.value("webcamWidth", 420).toInt());
    webcamHeightSpin_->setValue(s.value("webcamHeight", 236).toInt());
    subtitlesCheck_->setChecked(s.value("subtitlesEnabled", false).toBool());
    autoHighlightCheck_->setChecked(s.value("autoHighlightEnabled", false).toBool());
    whisperEdit_->setText(s.value("whisperCli", whisperEdit_->text()).toString());
    whisperModelEdit_->setText(s.value("whisperModel", QString()).toString());
    const int pos = webcamPositionCombo_->findData(s.value("webcamPosition", "br").toString());
    if (pos >= 0)
        webcamPositionCombo_->setCurrentIndex(pos);
}

void GamingCompanionDock::saveSettings()
{
    QSettings s(kOrg, kApp);
    s.setValue("duration", shortDuration());
    s.setValue("outputFolder", outputFolderEdit_->text());
    s.setValue("ffmpeg", ffmpegEdit_->text());
    s.setValue("logoPath", logoEdit_->text());
    s.setValue("logoEnabled", logoCheck_->isChecked());
    s.setValue("saveOriginal", saveOriginalCheck_->isChecked());
    s.setValue("webcamEnabled", webcamCheck_->isChecked());
    s.setValue("webcamWidth", webcamWidthSpin_->value());
    s.setValue("webcamHeight", webcamHeightSpin_->value());
    s.setValue("webcamPosition", webcamPositionCombo_->currentData().toString());
    s.setValue("subtitlesEnabled", subtitlesCheck_->isChecked());
    s.setValue("autoHighlightEnabled", autoHighlightCheck_->isChecked());
    s.setValue("whisperCli", whisperEdit_->text());
    s.setValue("whisperModel", whisperModelEdit_->text());
}

int GamingCompanionDock::shortDuration() const
{
    return durationCombo_->currentData().toInt();
}

void GamingCompanionDock::startReplayBuffer()
{
    if (!obs_frontend_replay_buffer_active())
        obs_frontend_replay_buffer_start();
    replayLabel_->setText(QStringLiteral("Replay-Puffer: wird gestartet …"));
}

void GamingCompanionDock::stopReplayBuffer()
{
    if (obs_frontend_replay_buffer_active())
        obs_frontend_replay_buffer_stop();
    replayLabel_->setText(QStringLiteral("Replay-Puffer: wird gestoppt …"));
}

void GamingCompanionDock::saveShort()
{
    if (!obs_frontend_replay_buffer_active()) {
        setStatus(QStringLiteral("Replay-Puffer ist nicht aktiv. Bitte zuerst starten."));
        return;
    }
    if (ffmpegProcess_.state() != QProcess::NotRunning) {
        setStatus(QStringLiteral("Es wird bereits ein Clip verarbeitet."));
        return;
    }

    pendingDuration_ = shortDuration();
    saveSettings();
    setStatus(QStringLiteral("Replay wird gespeichert …"));
    obs_frontend_replay_buffer_save();
}

void GamingCompanionDock::onReplaySaved(const QString &path)
{
    replayLabel_->setText(QStringLiteral("Replay gespeichert: %1").arg(QFileInfo(path).fileName()));
    if (autoHighlightCheck_->isChecked())
        analyzeHighlightCandidate(path);
    createClips(path);
}

void GamingCompanionDock::addMarker(const QString &type)
{
    QDir dir(outputFolderEdit_->text());
    dir.mkpath("Markers");
    QFile file(markerFilePath());
    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        setStatus(QStringLiteral("Marker-Datei konnte nicht geschrieben werden."));
        return;
    }

    QJsonObject obj;
    obj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    obj["type"] = type;
    obj["game"] = detectedGame_;
    obj["clipDuration"] = shortDuration();
    file.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    file.write("\n");
    file.close();
    setStatus(QStringLiteral("Marker gesetzt: %1").arg(type));
}

void GamingCompanionDock::browseOutputFolder()
{
    const QString folder = QFileDialog::getExistingDirectory(this, QStringLiteral("Ausgabeordner wählen"), outputFolderEdit_->text());
    if (!folder.isEmpty()) {
        outputFolderEdit_->setText(folder);
        saveSettings();
    }
}

void GamingCompanionDock::browseFfmpeg()
{
    const QString file = QFileDialog::getOpenFileName(this, QStringLiteral("FFmpeg auswählen"));
    if (!file.isEmpty()) {
        ffmpegEdit_->setText(file);
        saveSettings();
    }
}

void GamingCompanionDock::browseWhisper()
{
    const QString file = QFileDialog::getOpenFileName(this, QStringLiteral("whisper.cpp CLI auswählen"));
    if (!file.isEmpty()) {
        whisperEdit_->setText(file);
        saveSettings();
    }
}

void GamingCompanionDock::browseWhisperModel()
{
    const QString file = QFileDialog::getOpenFileName(this, QStringLiteral("Whisper-Modell auswählen"), QString(), QStringLiteral("Whisper Modelle (*.bin);;Alle Dateien (*)"));
    if (!file.isEmpty()) {
        whisperModelEdit_->setText(file);
        saveSettings();
    }
}

void GamingCompanionDock::browseLogo()
{
    const QString file = QFileDialog::getOpenFileName(this, QStringLiteral("Logo auswählen"), QString(), QStringLiteral("Bilder (*.png *.webp *.jpg *.jpeg)"));
    if (!file.isEmpty()) {
        logoEdit_->setText(file);
        logoCheck_->setChecked(true);
        saveSettings();
    }
}

QString GamingCompanionDock::sanitizedGameName() const
{
    QString name = detectedGame_.trimmed();
    if (name.isEmpty())
        name = QStringLiteral("Game");
    name.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9._-]+")), QStringLiteral("_"));
    return name.left(64);
}

QString GamingCompanionDock::shortOutputPath(const QString &inputFile) const
{
    QDir dir(outputFolderEdit_->text());
    dir.mkpath("Shorts");
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    return dir.filePath(QStringLiteral("Shorts/%1-short-%2s-%3.mp4").arg(sanitizedGameName()).arg(pendingDuration_).arg(stamp));
}

QString GamingCompanionDock::clipOutputPath(const QString &inputFile) const
{
    Q_UNUSED(inputFile)
    QDir dir(outputFolderEdit_->text());
    dir.mkpath("Clips");
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    return dir.filePath(QStringLiteral("Clips/%1-clip-%2s-%3.mp4").arg(sanitizedGameName()).arg(pendingDuration_).arg(stamp));
}

QString GamingCompanionDock::markerFilePath() const
{
    QDir dir(outputFolderEdit_->text());
    return dir.filePath(QStringLiteral("Markers/highlights-%1.jsonl").arg(QDate::currentDate().toString("yyyy-MM-dd")));
}

QString GamingCompanionDock::webcamCropFilter() const
{
    const int w = webcamWidthSpin_->value();
    const int h = webcamHeightSpin_->value();
    const QString pos = webcamPositionCombo_->currentData().toString();
    const int margin = 24;

    QString x = QString::number(margin);
    QString y = QString::number(margin);
    if (pos == "tr" || pos == "br")
        x = QStringLiteral("iw-%1-%2").arg(w).arg(margin);
    if (pos == "bl" || pos == "br")
        y = QStringLiteral("ih-%1-%2").arg(h).arg(margin);

    return QStringLiteral("crop=%1:%2:%3:%4").arg(w).arg(h).arg(x).arg(y);
}

void GamingCompanionDock::createClips(const QString &inputFile)
{
    if (ffmpegProcess_.state() != QProcess::NotRunning)
        return;

    const QString shortOut = shortOutputPath(inputFile);
    lastShortFile_ = shortOut;
    const QString clipOut = clipOutputPath(inputFile);
    const bool useLogo = logoCheck_->isChecked() && QFileInfo::exists(logoEdit_->text().trimmed());
    const bool useWebcam = webcamCheck_->isChecked();

    QStringList args;
    args << "-y" << "-sseof" << QStringLiteral("-%1").arg(pendingDuration_) << "-i" << inputFile;
    if (useLogo)
        args << "-i" << logoEdit_->text().trimmed();

    QString filter;
    if (useWebcam) {
        filter = QStringLiteral(
            "[0:v]split=3[camsrc][bgsrc][fgsrc];"
            "[bgsrc]scale=1080:1920:force_original_aspect_ratio=increase,crop=1080:1920,boxblur=20:10[bg];"
            "[fgsrc]scale=1080:1920:force_original_aspect_ratio=decrease[fg];"
            "[bg][fg]overlay=(W-w)/2:(H-h)/2[shortbase]");
    } else {
        filter = QStringLiteral(
            "[0:v]split=2[bgsrc][fgsrc];"
            "[bgsrc]scale=1080:1920:force_original_aspect_ratio=increase,crop=1080:1920,boxblur=20:10[bg];"
            "[fgsrc]scale=1080:1920:force_original_aspect_ratio=decrease[fg];"
            "[bg][fg]overlay=(W-w)/2:(H-h)/2[shortbase]");
    }

    QString currentShort = "[shortbase]";
    if (useWebcam) {
        filter += QStringLiteral(";[camsrc]%1,scale=720:-2,pad=iw+16:ih+16:8:8:white[cam];%2[cam]overlay=(W-w)/2:120[shortcam]")
                      .arg(webcamCropFilter(), currentShort);
        currentShort = "[shortcam]";
    }

    if (useLogo) {
        filter += QStringLiteral(";[1:v]scale=220:-2[logo];%1[logo]overlay=W-w-36:36[shortlogo]").arg(currentShort);
        currentShort = "[shortlogo]";
    }

    filter += QStringLiteral(";%1format=yuv420p[vshort]").arg(currentShort);

    args << "-filter_complex" << filter;

    // Vertical short output.
    args << "-map" << "[vshort]" << "-map" << "0:a?"
         << "-c:v" << "libx264" << "-preset" << "veryfast" << "-crf" << "20"
         << "-c:a" << "aac" << "-b:a" << "192k" << "-movflags" << "+faststart"
         << shortOut;

    // Exact same highlight in normal 16:9 as second output.
    if (saveOriginalCheck_->isChecked()) {
        args << "-map" << "0:v:0" << "-map" << "0:a?"
             << "-c:v" << "libx264" << "-preset" << "veryfast" << "-crf" << "18"
             << "-c:a" << "aac" << "-b:a" << "192k" << "-movflags" << "+faststart"
             << clipOut;
    }

    ffmpegProcess_.setProgram(ffmpegEdit_->text().trimmed());
    ffmpegProcess_.setArguments(args);
    ffmpegProcess_.start();
}

QString GamingCompanionDock::subtitlesOutputPath(const QString &shortFile) const
{
    QFileInfo fi(shortFile);
    return fi.dir().filePath(fi.completeBaseName() + QStringLiteral("-subtitles.mp4"));
}

void GamingCompanionDock::runSubtitlePipeline(const QString &shortFile)
{
    if (shortFile.isEmpty() || !QFileInfo::exists(shortFile)) {
        setStatus(QStringLiteral("Short gespeichert, aber Quelldatei für Untertitel fehlt."));
        return;
    }
    if (whisperEdit_->text().trimmed().isEmpty() || whisperModelEdit_->text().trimmed().isEmpty()) {
        setStatus(QStringLiteral("Short gespeichert. Für Untertitel bitte Whisper CLI und Modell auswählen."));
        return;
    }

    QDir temp(QDir(outputFolderEdit_->text()).filePath("Temp"));
    temp.mkpath(".");
    pendingAudioFile_ = temp.filePath(QFileInfo(shortFile).completeBaseName() + QStringLiteral("-whisper.wav"));

    QProcess *extract = new QProcess(this);
    connect(extract, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this, extract, shortFile](int code, QProcess::ExitStatus status) {
                extract->deleteLater();
                if (status == QProcess::NormalExit && code == 0)
                    runWhisper(pendingAudioFile_, shortFile);
                else
                    setStatus(QStringLiteral("Short gespeichert, Audio für Untertitel konnte nicht extrahiert werden."));
            });
    extract->start(ffmpegEdit_->text().trimmed(), {"-y", "-i", shortFile, "-vn", "-ac", "1", "-ar", "16000", "-c:a", "pcm_s16le", pendingAudioFile_});
    setStatus(QStringLiteral("Short gespeichert – Sprache wird lokal für Untertitel vorbereitet …"));
}

void GamingCompanionDock::runWhisper(const QString &audioFile, const QString &shortFile)
{
    Q_UNUSED(shortFile)
    QDir subs(QDir(outputFolderEdit_->text()).filePath("Subtitles"));
    subs.mkpath(".");
    pendingSrtBase_ = subs.filePath(QFileInfo(lastShortFile_).completeBaseName());
    QStringList args{"-m", whisperModelEdit_->text().trimmed(), "-f", audioFile, "-l", "auto", "-osrt", "-of", pendingSrtBase_};
    whisperProcess_.start(whisperEdit_->text().trimmed(), args);
    setStatus(QStringLiteral("Whisper erstellt lokale Untertitel …"));
}

void GamingCompanionDock::burnSubtitles(const QString &shortFile, const QString &srtFile)
{
    const QString out = subtitlesOutputPath(shortFile);
    QString escaped = QDir::fromNativeSeparators(srtFile);
    escaped.replace(":", "\\:");
    escaped.replace("'", "\\'");
    const QString vf = QStringLiteral("subtitles='%1':force_style='FontSize=18,Outline=2,Alignment=2,MarginV=120'").arg(escaped);
    subtitleProcess_.start(ffmpegEdit_->text().trimmed(), {"-y", "-i", shortFile, "-vf", vf, "-c:v", "libx264", "-preset", "veryfast", "-crf", "20", "-c:a", "copy", "-movflags", "+faststart", out});
    setStatus(QStringLiteral("Untertitel werden in den Short eingebrannt …"));
}

void GamingCompanionDock::analyzeHighlightCandidate(const QString &inputFile)
{
    if (highlightProcess_.state() != QProcess::NotRunning)
        return;

    // Generic games do not expose reliable kill events. This lightweight detector
    // records loud audio moments as highlight candidates; game-specific detectors
    // can later replace this heuristic without changing the marker format.
    QStringList args{"-hide_banner", "-sseof", QStringLiteral("-%1").arg(pendingDuration_), "-i", inputFile,
                     "-af", "silencedetect=noise=-18dB:d=0.20", "-f", "null", "-"};
    highlightProcess_.setProgram(ffmpegEdit_->text().trimmed());
    highlightProcess_.setArguments(args);
    highlightProcess_.setProcessChannelMode(QProcess::MergedChannels);
    connect(&highlightProcess_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int, QProcess::ExitStatus) {
                const QString log = QString::fromUtf8(highlightProcess_.readAll());
                const int transitions = log.count(QStringLiteral("silence_end"));
                if (transitions >= 3) {
                    QDir dir(outputFolderEdit_->text());
                    dir.mkpath("Markers");
                    QFile file(markerFilePath());
                    if (file.open(QIODevice::Append | QIODevice::Text)) {
                        QJsonObject obj;
                        obj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
                        obj["type"] = QStringLiteral("AutoHighlightCandidate");
                        obj["game"] = detectedGame_;
                        obj["audioTransitions"] = transitions;
                        obj["detector"] = QStringLiteral("generic-audio-v1");
                        file.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
                        file.write("\n");
                    }
                }
            }, Qt::UniqueConnection);
    highlightProcess_.start();
}

QString GamingCompanionDock::detectForegroundProcess() const
{
#ifdef Q_OS_WIN
    HWND hwnd = GetForegroundWindow();
    if (!hwnd)
        return {};
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (!pid)
        return {};

    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process)
        return {};

    wchar_t buffer[MAX_PATH * 4]{};
    DWORD size = static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0]));
    QString result;
    if (QueryFullProcessImageNameW(process, 0, buffer, &size))
        result = QFileInfo(QString::fromWCharArray(buffer, static_cast<int>(size))).completeBaseName();
    CloseHandle(process);
    return result;
#else
    return QStringLiteral("Nicht verfügbar");
#endif
}

void GamingCompanionDock::refreshDetectedGame()
{
    const QString process = detectForegroundProcess();
    if (process.isEmpty())
        return;

    // Avoid naming the plugin after common desktop/helper applications.
    const QString lower = process.toLower();
    static const QStringList ignored{ "obs64", "obs", "explorer", "chrome", "msedge", "firefox", "discord" };
    if (!ignored.contains(lower))
        detectedGame_ = process;

    gameLabel_->setText(detectedGame_.isEmpty()
                            ? QStringLiteral("Spiel: noch nicht erkannt")
                            : QStringLiteral("Erkanntes Spiel/Programm: <b>%1</b>").arg(detectedGame_.toHtmlEscaped()));
}

void GamingCompanionDock::setStatus(const QString &text)
{
    statusLabel_->setText(text);
}
