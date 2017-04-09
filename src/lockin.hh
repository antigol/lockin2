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

#ifndef LOCKIN_HPP
#define LOCKIN_HPP

#include <QAudioInput>
#include <QAudioFormat>
#include <QObject>
#include <QBuffer>
#include <QDataStream>
#include <QPair>
#include <QMutex>
#include <QVector>

class Fifo;

class Lockin : public QObject {
    Q_OBJECT
public:
    explicit Lockin(QObject *parent = 0);
    ~Lockin();

    bool isRunning() const;
    bool isFormatSupported(const QAudioFormat &format);

    // Cannot be called when running
    bool start(const QAudioDeviceInfo &audioDevice, const QAudioFormat &format);
    void setOutputPeriod(qreal outputPeriod);
    qreal outputPeriod() const;
    void setIntegrationTime(qreal integrationTime);
    qreal integrationTime() const;


    void setPhase(qreal phase);
    qreal phase() const;
    qreal autoPhase() const;
    QVector<QPair<qreal, qreal>> &raw_signals();
    const QAudioFormat &format() const;
    void stop();

signals:
    void newRawData();
    void newValues(qreal time, qreal x, qreal y);
    void info(QString msg);

private slots:
    void interpretInput();
    void audioStateChanged(QAudio::State state);

private:
	void readSoudCard(); // write into _left_right
	void parseChopperSignal(); // write into _sin_cos


    QAudioInput *_audioInput; // is null when lockin stoped
    Fifo *_fifo; // feeded by _audioInput

    QAudioFormat _format; // don't change it during running

    qreal _phase; // can be changed during running
    qreal _outputPeriod; // don't change it during running
    qreal _integrationTime; // don't change it during running
    int _sampleIntegration; // don't change it during running

    QVector<QPair<qreal, qreal>> _left_right; // raw signal
	QVector<QPair<qreal, qreal>> _sin_cos; // sin/cos constructed from right signal
    QList<QPair<qreal, qreal>> _dataXY; // product of left signal with sin/cos

    qreal _timeValue;
    qreal _xValue;
    qreal _yValue;
};

#endif // LOCKIN_HPP
