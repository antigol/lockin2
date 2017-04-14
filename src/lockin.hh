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
#include <complex>

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
    void setInvertLR(bool on);

    const QVector<QPair<qreal, qreal>> &raw_signals() const;
    const QVector<std::complex<qreal> > &complex_exp_signal() const;
    const QAudioFormat &format() const;
    void stop();

signals:
    void newRawData();
    void newValue(qreal time, qreal measure);

private slots:
    void interpretInput();

private:
	void readSoudCard(); // write into _left_right
    void parseChopperSignal(); // write into _complex_exp


    QAudioInput *_audioInput; // is null when lockin stoped
    Fifo *_fifo; // feeded by _audioInput

    QAudioFormat _format; // don't change it during running

    bool _invertLR;
    qreal _outputPeriod; // don't change it during running
    qreal _integrationTime; // don't change it during running
    int _sampleIntegration; // don't change it during running

    QVector<QPair<qreal, qreal>> _left_right; // raw signal
    QVector<std::complex<qreal>> _complex_exp; // sin/cos constructed from right signal
    QList<std::complex<qreal>> _measures; // product of left signal with sin/cos

    qreal _timeValue;
};

#endif // LOCKIN_HPP
