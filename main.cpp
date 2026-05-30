#include "mpvplayer.h"

#include <QCoreApplication>
#include <cstdio>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file> [file2 ...]\n", argv[0]);
        return 1;
    }

    MpvPlayer player;

    QObject::connect(&player, &MpvPlayer::initialized, [&]() {
        QStringList files;
        for (int i = 1; i < argc; ++i)
            files << QString::fromLocal8Bit(argv[i]);
        player.loadFiles(files);
    });

    QObject::connect(&player, &MpvPlayer::mediaTitleChanged, [](const QString &title) {
        printf("Now playing: %s\n", qPrintable(title));
        fflush(stdout);
    });

    QObject::connect(&player, &MpvPlayer::pauseChanged, [](bool paused) {
        printf(paused ? "[Paused]\n" : "[Playing]\n");
        fflush(stdout);
    });

    QObject::connect(&player, &MpvPlayer::durationChanged, [](double dur) {
        printf("Duration: %.1f s\n", dur);
        fflush(stdout);
    });

    QObject::connect(&player, &MpvPlayer::endOfFile, &app, &QCoreApplication::quit);

    QObject::connect(&player, &MpvPlayer::errorOccurred, [](const QString &msg) {
        fprintf(stderr, "Error: %s\n", qPrintable(msg));
        fflush(stderr);
    });

    if (!player.initialize(0))
        return 1;

    return app.exec();
}
