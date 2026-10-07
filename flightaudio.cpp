#include "flightaudio.h"

#include <QMediaPlayer>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QAudioOutput>
#else
#include <QMediaPlaylist>
#endif

FlightAudio::FlightAudio(QObject *parent)
    : QObject(parent), m_flight(new QMediaPlayer(this)), m_explosion(new QMediaPlayer(this)),
      m_critical(new QMediaPlayer(this))
{
    m_flight->setObjectName("flightSoundPlayer");
    m_explosion->setObjectName("explosionSoundPlayer");
    m_critical->setObjectName("criticalSoundPlayer");
    for (QMediaPlayer *player : {m_flight, m_explosion, m_critical}) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QAudioOutput *output = new QAudioOutput(player);
        output->setVolume(player == m_flight ? 0.35f : 0.65f);
        player->setAudioOutput(output);
        connect(player, &QMediaPlayer::errorOccurred, this, [this, player] {
            emit errorOccurred(tr("Ошибка звука: %1").arg(player->errorString()));
        });
#else
        player->setVolume(player == m_flight ? 35 : 65);
        connect(player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this,
                [this, player] { emit errorOccurred(tr("Ошибка звука: %1").arg(player->errorString())); });
#endif
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    m_flight->setLoops(QMediaPlayer::Infinite);
    m_flight->setSource(QUrl("qrc:/assets/drone_sound.mp3"));
    m_explosion->setSource(QUrl("qrc:/assets/exposion.mp3"));
    m_critical->setSource(QUrl("qrc:/assets/critical.mp3"));
#else
    QMediaPlaylist *playlist = new QMediaPlaylist(m_flight);
    playlist->addMedia(QUrl("qrc:/assets/drone_sound.mp3"));
    playlist->setPlaybackMode(QMediaPlaylist::CurrentItemInLoop);
    m_flight->setPlaylist(playlist);
    m_explosion->setMedia(QUrl("qrc:/assets/exposion.mp3"));
    m_critical->setMedia(QUrl("qrc:/assets/critical.mp3"));
#endif
}

FlightAudio::~FlightAudio()
{
    reset();
}

void FlightAudio::reset()
{
    m_state = Drone::State::Ready;
    m_flight->stop();
    m_explosion->stop();
    m_critical->stop();
}

void FlightAudio::playCritical()
{
    m_critical->stop();
    m_critical->setPosition(0);
    m_critical->play();
}

void FlightAudio::updateState(Drone::State state)
{
    if (state == m_state)
        return;
    m_state = state;
    switch (state) {
    case Drone::State::Running:
        m_critical->stop();
        m_explosion->stop();
        m_flight->play();
        break;
    case Drone::State::Paused:
        m_flight->pause();
        break;
    case Drone::State::Arrived:
        m_flight->stop();
        m_explosion->setPosition(0);
        m_explosion->play();
        break;
    case Drone::State::Ready:
        reset();
        break;
    }
}
