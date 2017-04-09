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
    _fifo = new Fifo(this);
    _fifo->open(QIODevice::ReadWrite);

    _audioInput = 0;

    setOutputPeriod(0.5);
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

    // nettoyage des variables
    _fifo->readAll(); // vide le fifo
    _dataXY.clear(); // vide <x,y>

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
    Q_ASSERT(_audioInput != 0);
	return (_phase + 0.1 * std::atan2(_yValue, _xValue)) * 180.0/M_PI;
}

QVector<QPair<qreal, qreal>> &Lockin::raw_signals()
{
    return _left_right;
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

    _left_right.clear();

    // load audio channels and cast them in the interval (-1, 1)
    readSoudCard();
	emit newRawData();

    if (_left_right.empty()) {
        qDebug() << __FUNCTION__ << ": empty channels";
        return;
    }

	parseChopperSignal();

    for (int i = 0; i < _left_right.size(); ++i) {
		qreal x = _sin_cos[i].first * _left_right[i].first; // sin
		qreal y = _sin_cos[i].second * _left_right[i].second; // cos

        if (!std::isnan(x) && !std::isnan(y)) {
            _dataXY << QPair<qreal, qreal>(x, y);
        }
    }

    // stop if there is not enough values into data xy
    if (_dataXY.size() < _sampleIntegration) {
        emit info("need more data for the integration time");
        return;
    }

    // remove the old unneeded values
    while (_dataXY.size() > _sampleIntegration)
        _dataXY.removeFirst();

    qreal x = 0.0;
    qreal y = 0.0;

    for (int i = 0; i < _dataXY.size(); ++i) {
        x += _dataXY[i].first;
        y += _dataXY[i].second;
    }

    x /= qreal(_sampleIntegration);
    y /= qreal(_sampleIntegration);

    _xValue = x;
    _yValue = y;


	emit newValues(_timeValue, x, y);


	static qreal total_time = 0.0;
    static int total_calls = 0;
    total_time += time.elapsed();
    total_calls += 1;
    qDebug() << __FUNCTION__ << ": execution time " << (total_time / total_calls) << "ms";
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

void Lockin::readSoudCard()
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
    QPair<qreal,qreal> pair;

    switch (_format.sampleType()) {
    case QAudioFormat::Float:
        while (!in.atEnd()) {
            in >> _float;
            pair.first = _float;
            in >> _float;
            pair.second = _float;
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
    const QPair<qreal, qreal> ignoredValue(NAN, NAN);

	_sin_cos.clear();

    int i = 0;

    // set the first value as ignored
	_sin_cos << ignoredValue;
    i++;

    for (; i < _left_right.size(); ++i) {
        if (_left_right[i-1].second < 0.0 && _left_right[i].second >= 0.0) {
            // first rising edge
            break;
        }
		_sin_cos << ignoredValue;
    }

    int periodSize = 0;
    for (; i < _left_right.size(); ++i) {
        periodSize++;
        if (_left_right[i-1].second < 0.0 && _left_right[i].second >= 0.0) {
            // rising edge

            for (int j = 0; j < periodSize; ++j) {
				qreal angle = 2.0 * M_PI * (qreal(j) / qreal(periodSize)) + _phase;
				_sin_cos << QPair<qreal, qreal>(std::sin(angle), std::cos(angle));
            }

            periodSize = 0;
        }
    }

    for (int j = 0; j < periodSize; ++j) {
		_sin_cos << ignoredValue;
    }

	Q_ASSERT(_sin_cos.size() == _left_right.size());
}
