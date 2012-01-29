#include "lockin2.hpp"
#include "qfifo.hpp"
#include <cmath>
#include <QDebug>
#include <QTime>

Lockin2::Lockin2(QObject *parent) :
    QObject(parent)
{
    //    _bufferWrite = new QBuffer(&_byteArray, this);
    //    _bufferWrite->open(QIODevice::WriteOnly | QIODevice::Unbuffered);

    //    _bufferRead = new QBuffer(&_byteArray, this);
    //    _bufferRead->open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    _fifo = new QFifo(this);
    _fifo->open(QIODevice::ReadWrite /*| QIODevice::Unbuffered*/);

    _audioInput = 0;

    setOutputPeriod(0.5);
    setVumeterTime(0.0);
    setIntegrationTime(3.0);
    setPhase(0.0);
}

Lockin2::~Lockin2()
{
    if (isRunning())
        stop();
}

bool Lockin2::isRunning() const
{
    return (_audioInput != 0);
}

bool Lockin2::isFormatSupported(const QAudioFormat &format)
{
    if (format.codec() != "audio/pcm") {
        return false;
    }

    if (format.channelCount() != 2) {
        return false;
    }

    return true;
}

bool Lockin2::start(const QAudioDeviceInfo &audioDevice, const QAudioFormat &format)
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

    if (audioDevice.isFormatSupported(format)) {
        _audioInput = new QAudioInput(audioDevice, format, this);
        _audioInput->setNotifyInterval(_outputPeriod * 1000.0);
        connect(_audioInput, SIGNAL(notify()), this, SLOT(interpretInput()));

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
    } else {
        qDebug() << __FUNCTION__ << ": format not supported, can't start";
        return false;
    }


    return true;
}

void Lockin2::setOutputPeriod(qreal outputPeriod)
{
    if (_audioInput == 0)
        _outputPeriod = outputPeriod;
    else
        qDebug() << __FUNCTION__ << ": lockin is running";
}

qreal Lockin2::outputPeriod() const
{
    return _outputPeriod;
}

void Lockin2::setIntegrationTime(qreal integrationTime)
{
    if (_audioInput == 0)
        _integrationTime = integrationTime;
    else
        qDebug() << __FUNCTION__ << ": lockin is running";
}

qreal Lockin2::integrationTime() const
{
    return _integrationTime;
}

void Lockin2::setVumeterTime(qreal vumeterTime)
{
    _vumeterMutex.lock();
    _vumeterTime = vumeterTime;
    _sampleVumeter = _vumeterTime * _format.sampleRate();
    _vumeterMutex.unlock();
}

void Lockin2::setPhase(qreal phase)
{
    _phase = phase * M_PI/180.0;
}

qreal Lockin2::phase() const
{
    return _phase * 180.0/M_PI;
}

qreal Lockin2::autoPhase() const
{
    if (_audioInput != 0) {
        return (_phase + std::atan2(_yValue, _xValue)) * 180.0/M_PI;
    } else {
        qDebug() << __FUNCTION__ << ": lockin is not running";
        return 0.0;
    }
}

QList<QPair<qreal, qreal> > Lockin2::vumeterData()
{
    _vumeterMutex.lock();
    QList<QPair<qreal, qreal> > ret = _vumeterData;
    _vumeterMutex.unlock();
    return ret;
}

const QAudioFormat &Lockin2::format() const
{
    return _format;
}

void Lockin2::stop()
{
    if (_audioInput != 0) {
        _audioInput->stop();
        delete _audioInput;
        _audioInput = 0;
    } else {
        qDebug() << __FUNCTION__ << ": lockin is not running";
    }
}

void Lockin2::interpretInput()
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
    QList<int> chopperSignal;

    // les des données et les adapte
    // left [-1;+1]
    // right -1 ou +1
    readSoudCard(leftSignal, chopperSignal);

    if (leftSignal.size() == 0 || chopperSignal.size() == 0)
        return;

    QList<QPair<qreal, qreal> > chopperSinCos = parseChopperSignal(chopperSignal, _phase);

    // multiplie le leftSignal ([-1;1]) par chopperSinCos ([-1;1])
    for (int i = 0; i < leftSignal.size(); ++i) {
        qreal x, y;
        if (chopperSinCos[i].first != 3.0) { // 3.0 est la valeur qui indique ignoreValue dans parseChopperSignal(...)
            x = chopperSinCos[i].first * leftSignal[i]; // sin
            y = chopperSinCos[i].second * leftSignal[i]; // cos
        } else {
            x = y = 3.0; // ignore value
        }
        _dataXY << QPair<qreal, qreal>(x, y);
    }

    // renouvelle le view data (prend la fin des listes, qui correspond aux parties les plus récentes)
    if (_vumeterMutex.tryLock()) {
        _vumeterData.clear();
        for (int i = leftSignal.size() - 1; i >= 0; --i) {
            if (_vumeterData.size() >= _sampleVumeter)
                break;

            if (chopperSinCos[i].first != 3.0) {
                _vumeterData.prepend(QPair<qreal, qreal>(leftSignal[i], chopperSinCos[i].first));
            }
        }
        _vumeterMutex.unlock();
    } else {
        qDebug() << __FUNCTION__ << ": the view Mutex is locked";
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

    qreal dataCount = 0.0;
    qreal x = 0.0;
    qreal y = 0.0;

    for (int i = 0; i < _dataXY.size(); ++i) {
        // deux fois 3.0 indique ignoreValue
        if (_dataXY[i].first != 3.0) {
            x += _dataXY[i].first;
            y += _dataXY[i].second;
            dataCount += 1.0;
        }
    }

    if (dataCount == 0.0) {
        qDebug() << __FUNCTION__ << ": no data usable";
        emit info("signal too low");
        return;
    }

    x /= dataCount;
    y /= dataCount;

    //    _values << QPair<qreal, QPair<qreal, qreal> >(_timeValue, QPair<qreal, qreal>(x, y));
    _xValue = x;
    _yValue = y;

    emit newValues(_timeValue, x, y);

    qDebug() << __FUNCTION__ << ": execution time " << time.elapsed() << "ms";
}

void Lockin2::readSoudCard(QList<qreal> &leftList, QList<int> &rightList)
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
            rightList << ((_float < 0) ? -1 : 1);
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
                rightList << ((_int8 < 0) ? -1 : 1);
                count++;
            }
            break;
        case 16:
            while (!in.atEnd()) {
                in >> _int16;
                leftList << qreal(_int16) / middle;
                in >> _int16;
                rightList << ((_int16 < 0) ? -1 : 1);
                count++;
            }
            break;
        case 32:
            while (!in.atEnd()) {
                in >> _int32;
                leftList << qreal(_int32) / middle;
                in >> _int32;
                rightList << ((_int32 < 0) ? -1 : 1);
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
                rightList << ((_uint8 < middle) ? -1 : 1);
                count++;
            }
            break;
        case 16:
            while (!in.atEnd()) {
                in >> _uint16;
                leftList << (qreal(_uint16) / middle) - 1.0;
                in >> _uint16;
                rightList << ((_uint16 < middle) ? -1 : 1);
                count++;
            }
            break;
        case 32:
            while (!in.atEnd()) {
                in >> _uint32;
                leftList << (qreal(_uint32) / middle) - 1.0;
                in >> _uint32;
                rightList << ((_uint32 < middle) ? -1 : 1);
                count++;
            }
            break;
        }
        break;
    case QAudioFormat::Unknown:
        break;
    }

    if (count == 0) {
        qDebug() << __FUNCTION__ << ": cannot read data";
    }
}

QList<QPair<qreal, qreal> > Lockin2::parseChopperSignal(QList<int> signal, qreal phase)
{
    // transforme le signal en sin et cos déphasé
    // dans les bords, la valeur 3.0 indique d'on ignore l'element

    QList<QPair<qreal, qreal> > out;

    bool ignore = true;
    bool wasBiggerThanAvg = (signal.first() == 1);
    int periodSize = 0;

    for (int i = 0; i < signal.size(); ++i) {
        bool isBiggerThanAvg = (signal[i] == 1);

        periodSize += 1;

        if (isBiggerThanAvg == true && wasBiggerThanAvg == false) {
            // flanc montant

            if (ignore) {
                // que pour les premières valeurs
                for (int i = 0; i < periodSize; ++i)
                    out << QPair<qreal, qreal>(3.0, 3.0);
                ignore = false;
            } else {
                // construire le sin et le cos
                for (int i = 0; i < periodSize; ++i) {
                    qreal angle = 2.0 * M_PI * (qreal(i) / qreal(periodSize)) + phase;
                    out << QPair<qreal, qreal>(std::sin(angle), std::cos(angle));
                }
            }

            periodSize = 0;
        }

        wasBiggerThanAvg = isBiggerThanAvg;
    }

    while (out.size() < signal.size())
        out << QPair<qreal, qreal>(3.0, 3.0);

    return out;
}
