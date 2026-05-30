#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <mpv/client.h>

class MpvPlayer : public QObject {
  Q_OBJECT

public:
  explicit MpvPlayer(QObject *parent = nullptr);
  ~MpvPlayer();

  bool initialize(qint64 wid);

  void loadFile(const QString &fileName, bool append = false);
  void loadFiles(const QStringList &files);
  void clearPlaylist();

  void togglePause();
  void setPause(bool pause);
  void stop();
  void seekAbsolute(double seconds);
  void seekRelative(double seconds);

  void setVolume(double volume);
  void setMuted(bool muted);
  void toggleMute();
  void setSpeed(double speed);

  bool isPaused() const { return m_paused; }
  bool isMuted() const { return m_muted; }
  double position() const { return m_position; }
  double duration() const { return m_duration; }
  double volume() const { return m_volume; }
  double speed() const { return m_speed; }
  QString mediaTitle() const { return m_mediaTitle; }
  QString currentPath() const { return m_currentPath; }

signals:
  void initialized();
  void fileLoaded();
  void pauseChanged(bool paused);
  void muteChanged(bool muted);
  void positionChanged(double seconds);
  void durationChanged(double seconds);
  void volumeChanged(double volume);
  void speedChanged(double speed);
  void mediaTitleChanged(const QString &title);
  void pathChanged(const QString &path);
  void endOfFile();
  void errorOccurred(const QString &message);

private slots:
  void processEvents();

private:
  static void wakeup(void *ctx);
  void observeProperties();
  void handleEvent(mpv_event *event);

  QString getStringProperty(const char *name) const;
  void refreshMetadata();

  mpv_handle *m_mpv = nullptr;

  bool m_paused = false;
  bool m_muted = false;
  double m_position = 0.0;
  double m_duration = 0.0;
  double m_volume = 100.0;
  double m_speed = 1.0;
  QString m_mediaTitle;
  QString m_currentPath;
};