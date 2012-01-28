#ifndef LOCKIN2_HPP
#define LOCKIN2_HPP

#include "lockin2_global.hpp"
#include "qfifo.hpp"

#include <QObject>
#include <QAudioInput>
#include <QAudioFormat>
#include <QBuffer>
#include <QDataStream>
#include <QPair>
#include <QMutex>

class LOCKIN2SHARED_EXPORT Lockin2 : public QObject {
    Q_OBJECT
public:
    explicit Lockin2(QObject *parent = 0);

    bool isRunning() const;
    bool isFormatSupported(const QAudioFormat &format);

    // peu être appelé quand sa tourne pas:
    bool start(const QAudioDeviceInfo &audioDevice, const QAudioFormat &format);
    void setOutputPeriod(qreal outputPeriod);
    qreal outputPeriod() const;
    void setIntegrationTime(qreal integrationTime);
    qreal integrationTime() const;
    void setVumeterTime(qreal vumeterTime); // le signal qui est sauvé pour être affiché à l'utilisateur

    // peu être appelé quand sa tourne:
    void setPhase(qreal phase);
    qreal phase() const;
    qreal autoPhase() const;
    QList<QPair<qreal, qreal> > vumeterData();

    void stop();

signals:
    void newValues(qreal time, qreal x, qreal y);

private slots:
    void interpretInput();

private:
    QList<QPair<qreal, qreal> > parseChopperSignal(QList<unsigned int> signal, quint32 avg, qreal phase);


    // vaut 0 quand s'est arrêté
    QAudioInput *_audioInput;

    /*
     * Peuvent être changés quand sa tourne
     */
    qreal _phase;


    /*
     * A ne pas toucher pandant que sa tourne
     */
    int _sampleSize;

//    QBuffer *_bufferWrite;
//    QBuffer *_bufferRead;
    QFifo *_fifo;
    QByteArray _byteArray;
    QDataStream::ByteOrder _byteOrder;

    QList<QPair<qreal, qreal> > _data;
    quint32 _avgRight;
    qreal _outputPeriod;

    qreal _integrationTime;
    int _sampleIntegration;

    qreal _vumeterTime;
    int _sampleVumeter;
    QMutex _vumeterMutex;
    QList<QPair<qreal, qreal> > _vumeterData; // <left, sin>

    qreal _timeValue;
//    QList<QPair<qreal, QPair<qreal, qreal> > > _values;
    qreal _xValue;
    qreal _yValue;
};

#endif // LOCKIN2_HPP
