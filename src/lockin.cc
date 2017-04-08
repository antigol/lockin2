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

#include "lockin.hh"
#include "fifo.hh"
#include <cstdio>
#include <cmath>
#include <QDebug>
#include <QTime>

Lockin::Lockin(QObject *parent) :
    QObject(parent)
{
    //    _bufferWrite = new QBuffer(&_byteArray, this);
    //    _bufferWrite->open(QIODevice::WriteOnly | QIODevice::Unbuffered);

    //    _bufferRead = new QBuffer(&_byteArray, this);
    //    _bufferRead->open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    _fifo = new Fifo(this);
    _fifo->open(QIODevice::ReadWrite /*| QIODevice::Unbuffered*/);

    _audioInput = 0;

    setOutputPeriod(0.5);
    setVumeterTime(0.0);
    setIntegrationTime(3.0);
    setPhase(0.0);
}

Lockin::~Lockin()
{
    if (isRunning())
        stop();
}

bool Lockin::isRunning() const
{
    return (_audioInput != 0);
}

bool Lockin::isFormatSupported(const QAudioFormat &format)
{
    if (format.codec() != "audio/pcm") {
        return false;
    }

    if (format.channelCount() != 2) {
        return false;
    }

    return true;
}

bool Lockin::start(const QAudioDeviceInfo &audioDevice, const QAudioFormat &format)
{
    if (_audioInput != 0) {
        qDebug() << __FUNCTION__ << ": lockin is already running, please stop is before start";
        return false;
    }

    if (!format.isValid()) {
        qDebug() << __FUNCTION__ << ": format not valid";
        return false;
    }

    if (!isFormatSupported(format)) {
        qDebug() << __FUNCTION__ << ": format not supported for lockin2";
        return false;
    }

    _audioInput = new QAudioInput(audioDevice, format, this);
    _audioInput->setNotifyInterval(_outputPeriod * 1000.0);

    connect(_audioInput, SIGNAL(notify()), this, SLOT(interpretInput()));
    connect(_audioInput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(audioStateChanged(QAudio::State)));

    // pour être au millieu avec le temps
    _timeValue = -(_integrationTime / 2.0);

    // nombre d'échantillons pour le temps d'integration
    _sampleIntegration = format.sampleRate() * _integrationTime;

    // nombre d'échantillons pour un affichage de vumeter
    _sampleVumeter = _vumeterTime * format.sampleRate();

    // nettoyage des variables
    _fifo->readAll(); // vide le fifo
    _dataXY.clear(); // vide <x,y>

    _format = format;

    _audioInput->start(_fifo);

    return true;
}

void Lockin::setOutputPeriod(qreal outputPeriod)
{
    if (_audioInput == 0)
        _outputPeriod = outputPeriod;
    else
        qDebug() << __FUNCTION__ << ": lockin is running";
}

qreal Lockin::outputPeriod() const
{
    return _outputPeriod;
}

void Lockin::setIntegrationTime(qreal integrationTime)
{
    if (_audioInput == 0)
        _integrationTime = integrationTime;
    else
        qDebug() << __FUNCTION__ << ": lockin is running";
}

qreal Lockin::integrationTime() const
{
    return _integrationTime;
}

void Lockin::setVumeterTime(qreal vumeterTime)
{
    _vumeterTime = vumeterTime;
    _sampleVumeter = _vumeterTime * _format.sampleRate();
}

void Lockin::setPhase(qreal phase)
{
    _phase = phase * M_PI/180.0;
}

qreal Lockin::phase() const
{
    return _phase * 180.0/M_PI;
}

qreal Lockin::autoPhase() const
{
    if (_audioInput != 0) {
        return (_phase + std::atan2(_yValue, _xValue)) * 180.0/M_PI;
    } else {
        qDebug() << __FUNCTION__ << ": lockin is not running";
        return 0.0;
    }
}

QList<QPair<qreal, qreal>> &Lockin::vumeterData()
{
    return _vumeterData;
}

const QAudioFormat &Lockin::format() const
{
    return _format;
}

void Lockin::stop()
{
    if (_audioInput != 0) {
        _audioInput->stop();
        delete _audioInput;
        _audioInput = 0;
    } else {
        qDebug() << __FUNCTION__ << ": lockin is not running";
    }
}

void Lockin::interpretInput()
{
    QTime time;
    time.start();

    _timeValue += _outputPeriod;

    // récupère les nouvelles valeurs
    /*
     * le nombre de nouvelle valeurs = outputPeriod * sampleRate / 1000
     * outputPeriod (~0.1 s) étant beaucoup plus grand que la periode du chopper (~1/500 s)
     * meme dans le cas d'un chopper lent et d'un temps de sortie rapide le nombre de periodes
     * du chopper reste elevé. (~50)
     * On peut donc garder uniquement les periodes entières
     * On a donc une perte de présision de l'ordre de 2/50 = 4%
     * Plus outputPeriod et grand et plus le chopper est rapide plus la perte diminue.
     * 0.5s 500Hz -> 0.8%
     * 1.0s 500Hz -> 0.4%
     */


    QList<qreal> leftSignal;
    QList<qreal> rightSignal;

    // load audio channels and cast them in the interval (-1, 1)
    readSoudCard(leftSignal, rightSignal);

    if (leftSignal.size() == 0 || rightSignal.size() == 0) {
        qDebug() << __FUNCTION__ << ": empty channels";
        return;
    }

    saveVumeterData(leftSignal, rightSignal);

    QList<QPair<qreal, qreal>> chopperSinCos = parseChopperSignal(rightSignal, _phase);

    // multiplie le leftSignal ([-1;1]) par chopperSinCos ([-1;1])
    for (int i = 0; i < leftSignal.size(); ++i) {
        qreal x = chopperSinCos[i].first * leftSignal[i]; // sin
        qreal y = chopperSinCos[i].second * leftSignal[i]; // cos
        _dataXY << QPair<qreal, qreal>(x, y);
    }

    // le nombre d'échantillons correslondant au temps d'integration
    if (_dataXY.size() < _sampleIntegration) {
        // pas encore assez d'elements pour faire la moyenne
        emit info("wait more input data...");
        return;
    }

    // supprime les valeurs en trop
    while (_dataXY.size() > _sampleIntegration)
        _dataXY.removeFirst();

    qreal n = 0.0;
    qreal x = 0.0;
    qreal y = 0.0;

    for (int i = 0; i < _dataXY.size(); ++i) {
        if (!std::isnan(_dataXY[i].first) && !std::isnan(_dataXY[i].second)) {
            x += _dataXY[i].first;
            y += _dataXY[i].second;
            n += 1.0;
        }
    }

    if (n == 0.0) {
        qDebug() << __FUNCTION__ << ": no data usable";
        emit info("signal too low");
        return;
    }

    x /= n;
    y /= n;

    //    _values << QPair<qreal, QPair<qreal, qreal> >(_timeValue, QPair<qreal, qreal>(x, y));
    _xValue = x;
    _yValue = y;

    emit newValues(_timeValue, x, y);

    qDebug() << __FUNCTION__ << ": execution time " << time.elapsed() << "ms";
}

void Lockin::audioStateChanged(QAudio::State state)
{
    switch (state) {
    case QAudio::ActiveState:
        qDebug() << __FUNCTION__ << ": ActiveState";
        break;
    case QAudio::SuspendedState:
        qDebug() << __FUNCTION__ << ": SuspendedState";
        break;
    case QAudio::StoppedState:
        qDebug() << __FUNCTION__ << ": StoppedState";
        break;
    case QAudio::IdleState:
        qDebug() << __FUNCTION__ << ": IdleState";
        break;
    }
}

void Lockin::saveVumeterData(const QList<qreal> &leftList, const QList<qreal> &rightList)
{
    if (leftList.size() >= _sampleVumeter) {
        _vumeterData.clear();
        _vumeterData.reserve(_sampleVumeter);
        for (int i = leftList.size() - _sampleVumeter; i < leftList.size(); ++i) {
            _vumeterData.append(QPair<qreal, qreal>(leftList[i], rightList[i]));
        }
        emit newVumeterData();
    }
}

void Lockin::readSoudCard(QList<qreal> &leftList, QList<qreal> &rightList)
{
    qreal middle = 0;

    switch (_format.sampleSize()) {
    case 8:
        middle = 128;
        break;
    case 16:
        middle = 32768;
        break;
    case 32:
        middle = 2147483648;
        break;
    }

    QDataStream in(_fifo);
    in.setByteOrder(QDataStream::ByteOrder(_format.byteOrder()));

    int count = 0;

    float _float;
    qint8 _int8;
    qint16 _int16;
    qint32 _int32;
    quint8 _uint8;
    quint16 _uint16;
    quint32 _uint32;

    switch (_format.sampleType()) {
    case QAudioFormat::Float:
        while (!in.atEnd()) {
            in >> _float;
            leftList << _float;
            in >> _float;
            rightList << _float;
            count++;
        }
        break;
    case QAudioFormat::SignedInt:
        switch (_format.sampleSize()) {
        case 8:
            while (!in.atEnd()) {
                in >> _int8;
                leftList << qreal(_int8) / middle;
                in >> _int8;
                rightList << qreal(_int8) / middle;
                count++;
            }
            break;
        case 16:
            while (!in.atEnd()) {
                in >> _int16;
                leftList << qreal(_int16) / middle;
                in >> _int16;
                rightList << qreal(_int16) / middle;
                count++;
            }
            break;
        case 32:
            while (!in.atEnd()) {
                in >> _int32;
                leftList << qreal(_int32) / middle;
                in >> _int32;
                rightList << qreal(_int32) / middle;
                count++;
            }
            break;
        }
        break;
    case QAudioFormat::UnSignedInt:
        switch (_format.sampleSize()) {
        case 8:
            while (!in.atEnd()) {
                in >> _uint8;
                leftList << (qreal(_uint8) / middle) - 1.0;
                in >> _uint8;
                rightList << (qreal(_uint8) / middle) - 1.0;
                count++;
            }
            break;
        case 16:
            while (!in.atEnd()) {
                in >> _uint16;
                leftList << (qreal(_uint16) / middle) - 1.0;
                in >> _uint16;
                rightList << (qreal(_uint16) / middle) - 1.0;
                count++;
            }
            break;
        case 32:
            while (!in.atEnd()) {
                in >> _uint32;
                leftList << (qreal(_uint32) / middle) - 1.0;
                in >> _uint32;
                rightList << (qreal(_uint32) / middle) - 1.0;
                count++;
            }
            break;
        }
        break;
    case QAudioFormat::Unknown:
        break;
    }

    if (count == 0) {
        qDebug() << __FUNCTION__ << ": cannot read data (in.atEnd = " << in.atEnd() << ")";
    }
}

QList<QPair<qreal, qreal>> Lockin::parseChopperSignal(const QList<qreal> &signal, qreal phase)
{
    const QPair<qreal, qreal> ignoredValue(NAN, NAN);

    QList<QPair<qreal, qreal>> out;

    int i = 0;

    // set the first value as ignored
    out << ignoredValue;
    i++;

    for (; i < signal.size(); ++i) {
        if (signal[i-1] < 0.0 && signal[i] >= 0.0) {
            // first rising edge
            break;
        }
        out << ignoredValue;
    }

    int periodSize = 0;
    for (; i < signal.size(); ++i) {
        periodSize++;
        if (signal[i-1] < 0.0 && signal[i] >= 0.0) {
            // rising edge

            for (int j = 0; j < periodSize; ++j) {
                qreal angle = 2.0 * M_PI * (qreal(j) / qreal(periodSize)) + phase;
                out << QPair<qreal, qreal>(std::sin(angle), std::cos(angle));
            }

            periodSize = 0;
        }
    }

    for (int j = 0; j < periodSize; ++j) {
        out << ignoredValue;
    }

    Q_ASSERT(out.size() == signal.size());
    return out;
}
