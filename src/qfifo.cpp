#include "qfifo.hpp"

QFifo::QFifo(QObject *parent) :
    QIODevice(parent)
{
}

bool QFifo::atEnd() const
{
    return (_data.isEmpty() && QIODevice::atEnd());
}

qint64 QFifo::bytesAvailable() const
{
    return _data.size() + QIODevice::bytesAvailable();
}

qint64 QFifo::readData(char *data, qint64 len)
{
    if ((len = qMin(len, qint64(_data.size()))) <= 0)
        return qint64(0);

    memcpy(data, _data.constData(), len);
    _data = _data.right(_data.size() - len);
    return len;
}

qint64 QFifo::writeData(const char *data, qint64 len)
{
    _data.append(data, len);
    return len;
}
