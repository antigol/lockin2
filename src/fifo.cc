#include "fifo.hh"

Fifo::Fifo(QObject *parent) :
    QIODevice(parent)
{
}

bool Fifo::atEnd() const
{
    return (_data.isEmpty() && QIODevice::atEnd());
}

qint64 Fifo::bytesAvailable() const
{
    return _data.size() + QIODevice::bytesAvailable();
}

bool Fifo::isSequential() const
{
    return true;
}

qint64 Fifo::readData(char *data, qint64 len)
{
    if ((len = qMin(len, qint64(_data.size()))) <= 0)
        return qint64(0);

    memcpy(data, _data.constData(), len);
    _data = _data.right(_data.size() - len); // Main difference with QBuffer
    return len;
}

qint64 Fifo::writeData(const char *data, qint64 len)
{
    _data.append(data, len);
    return len;
}
