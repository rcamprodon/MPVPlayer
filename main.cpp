#include <mpv/client.h>

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace {
int runCommand(mpv_handle *mpv, const char *const args[]) {
  const int result = mpv_command(mpv, args);
  if (result < 0) {
    std::cerr << "mpv command failed: " << mpv_error_string(result) << '\n';
  }
  return result;
}

void printHelp() {
  std::cout << "Commands:\n"
            << "  help               Show this help\n"
            << "  pause              Toggle pause\n"
            << "  play               Resume playback\n"
            << "  stop               Stop playback\n"
            << "  ff                 Seek +10 seconds\n"
            << "  rew                Seek -10 seconds\n"
            << "  seek <seconds>     Seek to absolute position\n"
            << "  vol <0-100>        Set volume\n"
            << "  speed <value>      Set playback speed\n"
            << "  mute               Toggle mute\n"
            << "  quit/exit          Exit\n";
}
} // namespace

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <media-file>\n";
    return 1;
  }

  mpv_handle *mpv = mpv_create();
  if (!mpv) {
    std::cerr << "mpv_create failed\n";
    return 1;
  }

  mpv_set_option_string(mpv, "video-sync", "display-resample");
  mpv_set_option_string(mpv, "interpolation", "yes");
  mpv_set_option_string(mpv, "keep-open", "yes");
  mpv_set_option_string(mpv, "hr-seek", "yes");
  mpv_set_option_string(mpv, "audio-wait-open", "no");

  if (mpv_initialize(mpv) < 0) {
    std::cerr << "mpv_initialize failed\n";
    mpv_terminate_destroy(mpv);
    return 1;
  }

  const char *loadFile[] = {"loadfile", argv[1], nullptr};
  if (runCommand(mpv, loadFile) < 0) {
    mpv_terminate_destroy(mpv);
    return 1;
  }

  printHelp();

  std::string line;
  while (true) {
    std::cout << "> ";
    if (!std::getline(std::cin, line))
      break;

    std::istringstream input(line);
    std::string cmd;
    input >> cmd;

    if (cmd.empty()) {
      continue;
    } else if (cmd == "help") {
      printHelp();
    } else if (cmd == "pause") {
      const char *args[] = {"cycle", "pause", nullptr};
      runCommand(mpv, args);
    } else if (cmd == "play") {
      int pauseFlag = 0;
      const int result =
          mpv_set_property(mpv, "pause", MPV_FORMAT_FLAG, &pauseFlag);
      if (result < 0) {
        std::cerr << "Failed to resume: " << mpv_error_string(result) << '\n';
      }
    } else if (cmd == "stop") {
      const char *args[] = {"stop", nullptr};
      runCommand(mpv, args);
    } else if (cmd == "ff") {
      const char *args[] = {"seek", "10", "relative", nullptr};
      runCommand(mpv, args);
    } else if (cmd == "rew") {
      const char *args[] = {"seek", "-10", "relative", nullptr};
      runCommand(mpv, args);
    } else if (cmd == "seek") {
      double seconds = 0.0;
      if (!(input >> seconds)) {
        std::cerr << "Usage: seek <seconds>\n";
        continue;
      }
      const int result =
          mpv_set_property(mpv, "time-pos", MPV_FORMAT_DOUBLE, &seconds);
      if (result < 0) {
        std::cerr << "Failed to seek: " << mpv_error_string(result) << '\n';
      }
    } else if (cmd == "vol") {
      double volume = 0.0;
      if (!(input >> volume)) {
        std::cerr << "Usage: vol <0-100>\n";
        continue;
      }
      if (volume < 0.0 || volume > 100.0) {
        std::cerr << "Volume must be between 0 and 100.\n";
        continue;
      }
      const int result = mpv_set_property(mpv, "volume", MPV_FORMAT_DOUBLE, &volume);
      if (result < 0) {
        std::cerr << "Failed to set volume: " << mpv_error_string(result) << '\n';
      }
    } else if (cmd == "speed") {
      double speed = 0.0;
      if (!(input >> speed)) {
        std::cerr << "Usage: speed <value>\n";
        continue;
      }
      const int result = mpv_set_property(mpv, "speed", MPV_FORMAT_DOUBLE, &speed);
      if (result < 0) {
        std::cerr << "Failed to set speed: " << mpv_error_string(result) << '\n';
      }
    } else if (cmd == "mute") {
      const char *args[] = {"cycle", "mute", nullptr};
      runCommand(mpv, args);
    } else if (cmd == "quit" || cmd == "exit") {
      break;
    } else {
      std::cerr << "Unknown command. Type 'help'.\n";
    }
  }

  mpv_terminate_destroy(mpv);
  return 0;
}
