#ifndef LOCKIN2_HPP
#define LOCKIN2_HPP

#include "lockin2_global.hpp"

#include <QObject>
#include <QAudioInput>
#include <QAudioFormat>
#include <QBuffer>
#include <QDataStream>
#include <QPair>
#include <QMutex>

class QFifo;

class LOCKIN2SHARED_EXPORT Lockin2 : public QObject {
    Q_OBJECT
public:
    explicit Lockin2(QObject *parent = 0);
    ~Lockin2();

    bool isRunning() const;
    bool isFormatSupported(const QAudioFormat &format);

    // peu être appelé quand sa tourne pas:
    bool start(const QAudioDeviceInfo &audioDevice, const QAudioFormat &format);
    void setOutputPeriod(qreal outputPeriod);
    qreal outputPeriod() const;
    void setIntegrationTime(qreal integrationTime);
    qreal integrationTime() const;

    // peu être appelé quand sa tourne:
    void setVumeterTime(qreal vumeterTime); // le signal qui est sauvé pour être affiché à l'utilisateur
    void setPhase(qreal phase);
    qreal phase() const;
    qreal autoPhase() const;
    QList<QPair<qreal, qreal> > vumeterData();
    const QAudioFormat &format() const;

    void stop();

signals:
    void newValues(qreal time, qreal x, qreal y);
    void info(QString msg);

private slots:
    void interpretInput();
    void audioStateChanged(QAudio::State state);

private:
    void readSoudCard(QList<qreal> &leftList, QList<int> &rightList);
    QList<QPair<qreal, qreal> > parseChopperSignal(QList<int> signal, qreal phase);


    // vaut 0 quand s'est arrêté
    QAudioInput *_audioInput;

    /*
     * Peuvent être changés quand sa tourne
     */
    qreal _phase;
    qreal _vumeterTime;
    int _sampleVumeter;
    QMutex _vumeterMutex;


    /*
     * A ne pas toucher pandant que sa tourne
     */
    QAudioFormat _format;

    QFifo *_fifo;
    QByteArray _byteArray;

    QList<QPair<qreal, qreal> > _dataXY;
    qreal _outputPeriod;

    qreal _integrationTime;
    int _sampleIntegration;

    QList<QPair<qreal, qreal> > _vumeterData; // <left, sin>

    qreal _timeValue;
//    QList<QPair<qreal, QPair<qreal, qreal> > > _values;
    qreal _xValue;
    qreal _yValue;
};

#endif // LOCKIN2_HPP
