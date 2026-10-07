#ifndef FLIGHTAUDIO_H
#define FLIGHTAUDIO_H

#include <QObject>
#include "drone.h"

class QMediaPlayer;

class FlightAudio : public QObject
{
    Q_OBJECT

public:
    explicit FlightAudio(QObject *parent = nullptr);
    ~FlightAudio() override;
    void updateState(Drone::State state);
    void reset();
    void playCritical();

signals:
    void errorOccurred(const QString &message);

private:
    QMediaPlayer *m_flight;
    QMediaPlayer *m_explosion;
    QMediaPlayer *m_critical;
    Drone::State m_state = Drone::State::Ready;
};

#endif
