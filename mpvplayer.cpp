#include "mpvplayer.h"

#include <QMetaObject>
#include <clocale>
#include <cstdint>

namespace {
constexpr uint64_t PROP_TIME_POS = 1;
constexpr uint64_t PROP_DURATION = 2;
constexpr uint64_t PROP_PAUSE = 3;
constexpr uint64_t PROP_VOLUME = 4;
constexpr uint64_t PROP_MUTE = 5;
constexpr uint64_t PROP_SPEED = 6;
constexpr uint64_t PROP_MEDIA_TITLE = 7;
constexpr uint64_t PROP_PATH = 8;
} // namespace

MpvPlayer::MpvPlayer(QObject *parent) : QObject(parent) {}

MpvPlayer::~MpvPlayer() {
  if (m_mpv)
    mpv_terminate_destroy(m_mpv);
}

bool MpvPlayer::initialize(qint64 wid) {
  // Support re-initialisation: clean up any existing instance first.
  if (m_mpv) {
    const char *stopCmd[] = {"stop", nullptr};
    (void)mpv_command(m_mpv, stopCmd); // best-effort stop before handle teardown
    mpv_terminate_destroy(m_mpv);
    m_mpv = nullptr;
    m_paused = false;
    m_muted = false;
    m_position = 0.0;
    m_duration = 0.0;
    m_volume = 100.0;
    m_speed = 1.0;
    m_mediaTitle.clear();
    m_currentPath.clear();
  }

  setlocale(LC_NUMERIC, "C");

  m_mpv = mpv_create();
  if (!m_mpv) {
    emit errorOccurred("mpv_create failed");
    return false;
  }

  mpv_set_option_string(m_mpv, "config", "no");
  mpv_set_option_string(m_mpv, "vo", "gpu");
  mpv_set_option_string(m_mpv, "hwdec", "no");

  int64_t windowId = static_cast<int64_t>(wid);
  if (mpv_set_option(m_mpv, "wid", MPV_FORMAT_INT64, &windowId) < 0) {
    emit errorOccurred("Failed to set mpv wid");
    mpv_destroy(m_mpv);
    m_mpv = nullptr;
    return false;
  }

  if (mpv_initialize(m_mpv) < 0) {
    emit errorOccurred("mpv_initialize failed");
    mpv_destroy(m_mpv);
    m_mpv = nullptr;
    return false;
  }

  observeProperties();
  mpv_set_wakeup_callback(m_mpv, &MpvPlayer::wakeup, this);

  emit initialized();
  return true;
}

void MpvPlayer::wakeup(void *ctx) {
  auto *self = static_cast<MpvPlayer *>(ctx);
  QMetaObject::invokeMethod(self, "processEvents", Qt::QueuedConnection);
}

void MpvPlayer::observeProperties() {
  mpv_observe_property(m_mpv, PROP_TIME_POS, "time-pos", MPV_FORMAT_DOUBLE);
  mpv_observe_property(m_mpv, PROP_DURATION, "duration", MPV_FORMAT_DOUBLE);
  mpv_observe_property(m_mpv, PROP_PAUSE, "pause", MPV_FORMAT_FLAG);
  mpv_observe_property(m_mpv, PROP_VOLUME, "volume", MPV_FORMAT_DOUBLE);
  mpv_observe_property(m_mpv, PROP_MUTE, "mute", MPV_FORMAT_FLAG);
  mpv_observe_property(m_mpv, PROP_SPEED, "speed", MPV_FORMAT_DOUBLE);
  mpv_observe_property(m_mpv, PROP_MEDIA_TITLE, "media-title",
                       MPV_FORMAT_STRING);
  mpv_observe_property(m_mpv, PROP_PATH, "path", MPV_FORMAT_STRING);
}

QString MpvPlayer::getStringProperty(const char *name) const {
  if (!m_mpv)
    return {};

  char *value = nullptr;
  if (mpv_get_property(m_mpv, name, MPV_FORMAT_STRING, &value) < 0 || !value)
    return {};

  QString result = QString::fromUtf8(value);
  mpv_free(value);
  return result;
}

void MpvPlayer::refreshMetadata() {
  const QString path = getStringProperty("path");
  if (path != m_currentPath) {
    m_currentPath = path;
    emit pathChanged(m_currentPath);
  }

  const QString title = getStringProperty("media-title");
  if (title != m_mediaTitle) {
    m_mediaTitle = title;
    emit mediaTitleChanged(m_mediaTitle);
  }
}

void MpvPlayer::loadFile(const QString &fileName, bool append) {
  if (!m_mpv || fileName.isEmpty())
    return;

  QByteArray utf8 = fileName.toUtf8();

  if (append) {
    const char *cmd[] = {"loadfile", utf8.constData(), "append-play", nullptr};
    mpv_command(m_mpv, cmd);
  } else {
    const char *cmd[] = {"loadfile", utf8.constData(), nullptr};
    mpv_command(m_mpv, cmd);
  }
}

void MpvPlayer::loadFiles(const QStringList &files) {
  bool first = true;
  for (const QString &file : files) {
    loadFile(file, !first);
    first = false;
  }
}

void MpvPlayer::clearPlaylist() {
  if (!m_mpv)
    return;

  const char *cmd[] = {"playlist-clear", nullptr};
  mpv_command(m_mpv, cmd);
}

void MpvPlayer::togglePause() {
  if (!m_mpv)
    return;

  const char *cmd[] = {"cycle", "pause", nullptr};
  mpv_command(m_mpv, cmd);
}

void MpvPlayer::setPause(bool pause) {
  if (!m_mpv)
    return;

  int flag = pause ? 1 : 0;
  mpv_set_property(m_mpv, "pause", MPV_FORMAT_FLAG, &flag);
}

void MpvPlayer::stop() {
  if (!m_mpv)
    return;

  const char *cmd[] = {"stop", nullptr};
  mpv_command(m_mpv, cmd);
}

void MpvPlayer::seekAbsolute(double seconds) {
  if (!m_mpv)
    return;

  QByteArray sec = QByteArray::number(seconds, 'f', 3);
  const char *cmd[] = {"seek", sec.constData(), "absolute", nullptr};
  mpv_command(m_mpv, cmd);
}

void MpvPlayer::seekRelative(double seconds) {
  if (!m_mpv)
    return;

  QByteArray sec = QByteArray::number(seconds, 'f', 3);
  const char *cmd[] = {"seek", sec.constData(), "relative", "exact", nullptr};
  mpv_command(m_mpv, cmd);
}

void MpvPlayer::setVolume(double volume) {
  if (!m_mpv)
    return;

  double v = volume;
  mpv_set_property(m_mpv, "volume", MPV_FORMAT_DOUBLE, &v);
}

void MpvPlayer::setMuted(bool muted) {
  if (!m_mpv)
    return;

  int flag = muted ? 1 : 0;
  mpv_set_property(m_mpv, "mute", MPV_FORMAT_FLAG, &flag);
}

void MpvPlayer::toggleMute() {
  if (!m_mpv)
    return;

  const char *cmd[] = {"cycle", "mute", nullptr};
  mpv_command(m_mpv, cmd);
}

void MpvPlayer::setSpeed(double speed) {
  if (!m_mpv)
    return;

  double s = speed;
  mpv_set_property(m_mpv, "speed", MPV_FORMAT_DOUBLE, &s);
}

void MpvPlayer::processEvents() {
  if (!m_mpv)
    return;

  while (true) {
    mpv_event *event = mpv_wait_event(m_mpv, 0);
    if (!event || event->event_id == MPV_EVENT_NONE)
      break;

    handleEvent(event);
  }
}

void MpvPlayer::handleEvent(mpv_event *event) {
  switch (event->event_id) {
  case MPV_EVENT_FILE_LOADED:
    refreshMetadata();
    emit fileLoaded();
    break;

  case MPV_EVENT_END_FILE:
    emit endOfFile();
    break;

  case MPV_EVENT_PROPERTY_CHANGE: {
    auto *prop = static_cast<mpv_event_property *>(event->data);
    if (!prop)
      break;

    switch (event->reply_userdata) {
    case PROP_TIME_POS:
      if (prop->format == MPV_FORMAT_DOUBLE && prop->data) {
        m_position = *static_cast<double *>(prop->data);
        emit positionChanged(m_position);
      }
      break;

    case PROP_DURATION:
      if (prop->format == MPV_FORMAT_DOUBLE && prop->data) {
        m_duration = *static_cast<double *>(prop->data);
        emit durationChanged(m_duration);
      }
      break;

    case PROP_PAUSE:
      if (prop->format == MPV_FORMAT_FLAG && prop->data) {
        m_paused = (*static_cast<int *>(prop->data) != 0);
        emit pauseChanged(m_paused);
      }
      break;

    case PROP_VOLUME:
      if (prop->format == MPV_FORMAT_DOUBLE && prop->data) {
        m_volume = *static_cast<double *>(prop->data);
        emit volumeChanged(m_volume);
      }
      break;

    case PROP_MUTE:
      if (prop->format == MPV_FORMAT_FLAG && prop->data) {
        m_muted = (*static_cast<int *>(prop->data) != 0);
        emit muteChanged(m_muted);
      }
      break;

    case PROP_SPEED:
      if (prop->format == MPV_FORMAT_DOUBLE && prop->data) {
        m_speed = *static_cast<double *>(prop->data);
        emit speedChanged(m_speed);
      }
      break;

    case PROP_MEDIA_TITLE:
    case PROP_PATH:
      refreshMetadata();
      break;
    }
    break;
  }

  default:
    break;
  }
}