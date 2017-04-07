/****************************************************************************
**
**  Copyright (C) 2015 Mario Geiger
**  Contact: geiger.mario@gmail.com
**
**  This file is part of lockin2.
**
**  lockin2 is free software: you can redistribute it and/or modify
**  it under the terms of the GNU Lesser General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  lockin2 is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU Lesser General Public License for more details.
**
**  You should have received a copy of the GNU Lesser General Public License
**  along with lockin2.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#ifndef LOCKIN2_HPP
#define LOCKIN2_HPP

#include <QAudioInput>
#include <QAudioFormat>
#include <QObject>
#include <QBuffer>
#include <QDataStream>
#include <QPair>
#include <QMutex>

class QFifo;

class Lockin2 : public QObject {
    Q_OBJECT
public:
    explicit Lockin2(QObject *parent = 0);
    ~Lockin2();

    bool isRunning() const;
    bool isFormatSupported(const QAudioFormat &format);

    // peu être appelé quand sa ne tourne pas:
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


    // vaut 0 quand c'est arrêté
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
