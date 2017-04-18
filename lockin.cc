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
#include <cmath>
#include <QDebug>
#include <QDataStream>

Lockin::Lockin(QObject *parent) :
    QObject(parent)
{
    _fifo = new Fifo(this);
    _fifo->open(QIODevice::ReadWrite);

    _audioInput = nullptr;

    _invertLR = false;
    setOutputPeriod(0.5);
    setIntegrationTime(3.0);
}

Lockin::~Lockin()
{
    if (isRunning())
        stop();
}

bool Lockin::isRunning() const
{
    return _audioInput != nullptr;
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

    // pour être au millieu avec le temps
    _timeValue = -(_integrationTime / 2.0);

    // nombre d'échantillons pour le temps d'integration
    _sampleIntegration = format.sampleRate() * _integrationTime;

    // nettoyage des variables
    _fifo->readAll(); // vide le fifo
    _measures.clear(); // vide <x,y>

    _format = format;

    _audioInput->start(_fifo);

    return true;
}

void Lockin::setOutputPeriod(qreal outputPeriod)
{
    Q_ASSERT(_audioInput == 0);
    _outputPeriod = outputPeriod;
}

qreal Lockin::outputPeriod() const
{
    return _outputPeriod;
}

void Lockin::setIntegrationTime(qreal integrationTime)
{
    Q_ASSERT(_audioInput == 0);
    _integrationTime = integrationTime;
}

qreal Lockin::integrationTime() const
{
    return _integrationTime;
}

void Lockin::setInvertLR(bool on)
{
    _invertLR = on;
}

const QVector<QPair<qreal, qreal>> &Lockin::raw_signals() const
{
    return _left_right;
}

const QVector<std::complex<qreal> > &Lockin::complex_exp_signal() const
{
    return _complex_exp;
}

const QAudioFormat &Lockin::format() const
{
    return _format;
}

void Lockin::stop()
{
    if (_audioInput != nullptr) {
        _audioInput->stop();
        delete _audioInput;
        _audioInput = nullptr;
    } else {
        qDebug() << __FUNCTION__ << ": lockin is not running";
    }
}

void Lockin::interpretInput()
{
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

    _left_right.clear();

    // load audio channels and cast them in the interval (-1, 1)
    readSoudCard();

    if (_left_right.empty()) {
        qDebug() << __FUNCTION__ << ": empty channels";
        return;
    }

	parseChopperSignal();
    emit newRawData();

    for (int i = 0; i < _left_right.size(); ++i) {
        std::complex<qreal> x = _complex_exp[i] * _left_right[i].first;

        if (!std::isnan(x.real()) && !std::isnan(x.imag())) {
            _measures << x;
        }
    }

    // stop if there is not enough values into data xy
    if (_measures.size() < _sampleIntegration) {
        return;
    }

    // remove the old unneeded values
    while (_measures.size() > _sampleIntegration)
        _measures.removeFirst();

    std::complex<qreal> x = 0.0;

    for (int i = 0; i < _measures.size(); ++i) {
        x += _measures[i];
    }

    x /= qreal(_sampleIntegration);

    emit newValue(_timeValue, std::abs(x));
}

void Lockin::readSoudCard()
{
    qreal middle = 0;

    switch (_format.sampleSize()) {
    case 8:
        middle = 128;
        Q_ASSERT(_fifo->bytesAvailable() % 2 == 0);
        break;
    case 16:
        middle = 32768;
        Q_ASSERT((_fifo->bytesAvailable() / 2) % 2 == 0);
        break;
    case 32:
        middle = 2147483648;
        Q_ASSERT((_fifo->bytesAvailable() / 4) % 2 == 0);
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
    QPair<qreal,qreal> pair;

    switch (_format.sampleType()) {
    case QAudioFormat::Float:
        while (!in.atEnd()) {
            in >> _float;
            pair.first = _float;
            in >> _float;
            pair.second = _float;
            if (_invertLR) {
                std::swap(pair.first, pair.second);
            }
            _left_right.append(pair);
            count++;
        }
        break;
    case QAudioFormat::SignedInt:
        switch (_format.sampleSize()) {
        case 8:
            while (!in.atEnd()) {
                in >> _int8;
                pair.first = qreal(_int8) / middle;
                in >> _int8;
                pair.second = qreal(_int8) / middle;
                if (_invertLR) {
                    std::swap(pair.first, pair.second);
                }
                _left_right.append(pair);
                count++;
            }
            break;
        case 16:
            while (!in.atEnd()) {
                in >> _int16;
                pair.first = qreal(_int16) / middle;
                in >> _int16;
                pair.second = qreal(_int16) / middle;
                if (_invertLR) {
                    std::swap(pair.first, pair.second);
                }
                _left_right.append(pair);
                count++;
            }
            break;
        case 32:
            while (!in.atEnd()) {
                in >> _int32;
                pair.first = qreal(_int32) / middle;
                in >> _int32;
                pair.second = qreal(_int32) / middle;
                if (_invertLR) {
                    std::swap(pair.first, pair.second);
                }
                _left_right.append(pair);
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
                pair.first = (qreal(_uint8) / middle) - 1.0;
                in >> _uint8;
                pair.second = (qreal(_uint8) / middle) - 1.0;
                if (_invertLR) {
                    std::swap(pair.first, pair.second);
                }
                _left_right.append(pair);
                count++;
            }
            break;
        case 16:
            while (!in.atEnd()) {
                in >> _uint16;
                pair.first = (qreal(_uint16) / middle) - 1.0;
                in >> _uint16;
                pair.second = (qreal(_uint16) / middle) - 1.0;
                if (_invertLR) {
                    std::swap(pair.first, pair.second);
                }
                _left_right.append(pair);
                count++;
            }
            break;
        case 32:
            while (!in.atEnd()) {
                in >> _uint32;
                pair.first = (qreal(_uint32) / middle) - 1.0;
                in >> _uint32;
                pair.second = (qreal(_uint32) / middle) - 1.0;
                if (_invertLR) {
                    std::swap(pair.first, pair.second);
                }
                _left_right.append(pair);
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

void Lockin::parseChopperSignal()
{
    _complex_exp.clear();

    int i = 0;

    // set the first value as ignored
    _complex_exp << NAN;
    i++;

    for (; i < _left_right.size(); ++i) {
        if (_left_right[i-1].second < 0.0 && _left_right[i].second >= 0.0) {
            // first rising edge
            break;
        }
        _complex_exp << NAN;
    }

    int periodSize = 0;
    for (; i < _left_right.size(); ++i) {
        periodSize++;
        if (_left_right[i-1].second < 0.0 && _left_right[i].second >= 0.0) {
            // rising edge

            for (int j = 0; j < periodSize; ++j) {
                qreal angle = 2.0 * M_PI * qreal(j) / qreal(periodSize);
                _complex_exp << std::exp(std::complex<qreal>(0.0, 1.0) * angle);
            }

            periodSize = 0;
        }
    }

    for (int j = 0; j < periodSize; ++j) {
        _complex_exp << NAN;
    }

    Q_ASSERT(_complex_exp.size() == _left_right.size());
}
