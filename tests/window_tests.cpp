#include "mainwindow.h"
#include "explosioneffect.h"
#include "videotexture.h"
#include "flightaudio.h"

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFile>
#include <QSlider>
#include <QSignalSpy>
#include <QMediaPlayer>
#include <QTest>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DRender/QFrameGraphNode>
#include <Qt3DRender/QNoDraw>
#include <Qt3DRender/QRenderCapture>
#include <cstdlib>
#include <iostream>

namespace {
auto playbackState(QMediaPlayer *player)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return player->playbackState();
#else
    return player->state();
#endif
}

void check(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    check(!QImage(":/assets/drone.jpg").isNull(), "drone image is embedded and readable");
    check(QFile::exists(":/assets/drone_sound.mp3") && QFile::exists(":/assets/exposion.mp3"),
          "flight and explosion sound assets are embedded");
    QImage sample(3, 1, QImage::Format_ARGB32);
    sample.setPixel(0, 0, qRgb(0, 205, 10));
    sample.setPixel(1, 0, qRgb(255, 180, 30));
    sample.setPixel(2, 0, qRgb(120, 165, 90));
    const QImage keyed = VideoTexture::removeGreenScreen(sample);
    check(qAlpha(keyed.pixel(0, 0)) == 0, "green video background is transparent");
    check(keyed.pixel(1, 0) == sample.pixel(1, 0), "fire colors remain opaque");
    check(qAlpha(keyed.pixel(2, 0)) > 0 && qAlpha(keyed.pixel(2, 0)) < 255,
          "green edge has soft transparency");
    MainWindow window;
    ExplosionEffect *explosion = window.findChild<ExplosionEffect *>();
    check(explosion && !explosion->isPlaying(), "explosion is idle before arrival");
    QSignalSpy frames(explosion, &ExplosionEffect::frameReady);
    QSignalSpy videoErrors(explosion, &ExplosionEffect::errorOccurred);
    FlightAudio *audio = window.findChild<FlightAudio *>();
    check(audio, "audio controller exists");
    QMediaPlayer *flightSound = audio->findChild<QMediaPlayer *>("flightSoundPlayer");
    QMediaPlayer *explosionSound = audio->findChild<QMediaPlayer *>("explosionSoundPlayer");
    QMediaPlayer *criticalSound = audio->findChild<QMediaPlayer *>("criticalSoundPlayer");
    check(flightSound && explosionSound && criticalSound, "separate flight, explosion and alert players");
    QSignalSpy audioErrors(audio, &FlightAudio::errorOccurred);
    QPushButton *plan = window.findChild<QPushButton *>("btnRoute");
    QPushButton *fly = window.findChild<QPushButton *>("btnStartPause");
    QPushButton *reset = window.findChild<QPushButton *>("btnReset");
    QLabel *error = window.findChild<QLabel *>("labelInputError");
    QLabel *route = window.findChild<QLabel *>("labelRoute");
    QLabel *length = window.findChild<QLabel *>("labelLength");
    QLabel *status = window.findChild<QLabel *>("labelStatus");
    QLineEdit *startX = window.findChild<QLineEdit *>("lineEditStartX");
    QLineEdit *startZ = window.findChild<QLineEdit *>("lineEditStartZ");
    QLineEdit *goalZ = window.findChild<QLineEdit *>("lineEditGoalZ");
    QLineEdit *radius = window.findChild<QLineEdit *>("lineEditRadius");
    QSlider *speed = window.findChild<QSlider *>("sliderSpeed");
    check(plan && fly && reset && error && route && length && status
          && startX && startZ && goalZ && radius && speed, "required controls exist");
    check(!fly->isEnabled(), "flight disabled before route planning");
    plan->click();
    check(error->text().isEmpty() && fly->isEnabled(), "default route builds");
    check(route->text().contains(QString::fromUtf8("Обход")), "default route is a detour");
    check(!length->text().contains(QString::fromUtf8("—")), "route length shown");

    // Optional real-display smoke test: capture the 3D framebuffer with the effect.
    const QString screenshot = qEnvironmentVariable("EW_TEST_SCREENSHOT");
    if (!screenshot.isEmpty()) {
        window.show();
        QTest::qWait(2500);
    }

    fly->click();
    check(fly->text() == QString::fromUtf8("Пауза"), "start activates flight");
    check(QTest::qWaitFor([flightSound] {
        return playbackState(flightSound) == QMediaPlayer::PlayingState;
    }, 5000), "flight starts drone sound");
    QTest::qWait(80);
    fly->click();
    const QString pausedStatus = status->text();
    check(pausedStatus.contains(QString::fromUtf8("Пауза")), "pause displayed");
    check(playbackState(flightSound) == QMediaPlayer::PausedState, "pause also pauses drone sound");
    QTest::qWait(80);
    check(status->text() == pausedStatus, "telemetry remains fixed during pause");

    // Simulate actual editing, including the textEdited signal.
    startX->setFocus();
    startX->selectAll();
    QTest::keyClicks(startX, "70");
    check(!fly->isEnabled(), "editing invalidates route");
    check(playbackState(flightSound) == QMediaPlayer::StoppedState, "route editing stops drone sound");
    check(length->text().contains(QString::fromUtf8("—")), "editing clears length");
    plan->click();
    check(fly->isEnabled(), "replan after editing");

    radius->setText("0");
    plan->click();
    check(!error->text().isEmpty() && !fly->isEnabled(), "invalid radius prevents flight");
    check(QTest::qWaitFor([criticalSound] {
        return playbackState(criticalSound) == QMediaPlayer::PlayingState;
    }, 5000), "invalid numeric input plays critical sound");
    reset->click();
    check(playbackState(criticalSound) == QMediaPlayer::StoppedState, "reset stops critical sound");
    startX->setFocus();
    QTest::keyClicks(startX, "a");
    check(QTest::qWaitFor([criticalSound] {
        return playbackState(criticalSound) == QMediaPlayer::PlayingState;
    }, 5000), "rejected coordinate character plays critical sound");
    reset->click();
    check(startX->text() == "50" && radius->text() == "120", "reset restores fields");
    check(error->text().isEmpty() && !fly->isEnabled(), "reset clears error and route");
    speed->setValue(200);
    reset->click();
    check(speed->value() == 80, "reset restores speed");

    startX->setText("300");
    window.findChild<QLineEdit *>("lineEditStartY")->setText("200");
    startZ->setText("200");
    plan->click();
    check(!error->text().isEmpty() && !fly->isEnabled(), "start inside sphere is rejected");
    check(QTest::qWaitFor([criticalSound] {
        return playbackState(criticalSound) == QMediaPlayer::PlayingState;
    }, 5000), "coordinate inside zone plays critical sound");
    reset->click();
    startZ->setText("500");
    goalZ->setText("500");
    plan->click();
    check(route->text().contains(QString::fromUtf8("Прямой")), "high-altitude direct route");

    // Coincident points arrive on the first tick and can be replayed.
    reset->click();
    window.findChild<QLineEdit *>("lineEditGoalX")->setText("50");
    window.findChild<QLineEdit *>("lineEditGoalY")->setText("350");
    goalZ->setText("100");
    plan->click();
    fly->click();
    check(QTest::qWaitFor([status] {
        return status->text().contains(QString::fromUtf8("Цель достигнута"));
    }, 5000), "arrival displayed");
    check(explosion->isPlaying(), "arrival starts explosion");
    check(playbackState(flightSound) == QMediaPlayer::StoppedState, "arrival stops drone sound");
    check(QTest::qWaitFor([explosionSound] {
        return playbackState(explosionSound) == QMediaPlayer::PlayingState;
    }, 5000), "arrival plays explosion sound");
    check(QTest::qWaitFor([&frames] { return frames.count() >= 3; }, 10000),
          "embedded MP4 decodes frames");
    check(videoErrors.isEmpty(), "video plays without errors");
    if (!screenshot.isEmpty()) {
        check(QTest::qWaitFor([&frames] { return frames.count() >= 35; }, 10000), "explosion develops");
        QMediaPlayer *player = explosion->findChild<QMediaPlayer *>();
        player->pause();
        Qt3DRender::QRenderCapture *capture = nullptr;
        for (QWindow *view : QGuiApplication::allWindows()) {
            Qt3DExtras::Qt3DWindow *scene = qobject_cast<Qt3DExtras::Qt3DWindow *>(view);
            if (!scene)
                continue;
            const auto nodes = scene->activeFrameGraph()->findChildren<Qt3DRender::QFrameGraphNode *>();
            for (Qt3DRender::QFrameGraphNode *node : nodes) {
                if (!qobject_cast<Qt3DRender::QNoDraw *>(node)
                    && node->findChildren<Qt3DRender::QFrameGraphNode *>(
                        QString(), Qt::FindDirectChildrenOnly).isEmpty()) {
                    capture = new Qt3DRender::QRenderCapture(node);
                    break;
                }
            }
        }
        check(capture, "3D render branch found");
        Qt3DRender::QRenderCaptureReply *reply = capture->requestCapture();
        check(QTest::qWaitFor([reply] { return reply->isComplete(); }, 10000), "3D screenshot captured");
        check(reply->image().save(screenshot), "save explosion rendering");
        delete reply;
        delete capture;
        player->play();
    }
    check(QTest::qWaitFor([explosion] { return !explosion->isPlaying(); }, 10000),
          "explosion finishes once");
    speed->setValue(100);
    check(!explosion->isPlaying(), "telemetry refresh does not replay finished explosion");
    check(QTest::qWaitFor([explosionSound] {
        return playbackState(explosionSound) == QMediaPlayer::StoppedState;
    }, 10000), "explosion sound finishes once");
    speed->setValue(110);
    check(playbackState(explosionSound) == QMediaPlayer::StoppedState,
          "telemetry refresh does not replay explosion sound");
    check(audioErrors.isEmpty(), "audio assets decode without errors");
    frames.clear();
    fly->click();
    check(fly->text() == QString::fromUtf8("Пауза"), "replay starts again");
    check(QTest::qWaitFor([&frames] { return !frames.isEmpty(); }, 10000),
          "next arrival replays video from the beginning");
    reset->click();
    check(!explosion->isPlaying(), "reset cancels explosion");
    check(playbackState(flightSound) == QMediaPlayer::StoppedState
          && playbackState(explosionSound) == QMediaPlayer::StoppedState, "reset stops both sounds");
    frames.clear();
    QTest::qWait(100);
    check(frames.isEmpty(), "late decoder frames do not restore a cancelled explosion");
    window.close();
    std::cout << "Window interaction tests passed.\n";
}
