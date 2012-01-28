#ifndef QFIFO_HPP
#define QFIFO_HPP

#include <QIODevice>
#include <QByteArray>

class QFifo : public QIODevice
{
    Q_OBJECT
public:
    explicit QFifo(QObject *parent = 0);
    
    bool atEnd() const;
    qint64 bytesAvailable() const;

private:
    qint64 readData(char *data, qint64 len);
    qint64 writeData(const char *data, qint64 len);

    QByteArray _data;
};

#endif // QFIFO_HPP
