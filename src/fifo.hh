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

#ifndef FIFO_HPP
#define FIFO_HPP

#include <QIODevice>
#include <QByteArray>

/* This class is similar to QBuffer
 * Except that it always read at the begining of the ByteArray
 * and free the memory after reading it
 *
 * Also it always write at the end
 */

class Fifo : public QIODevice
{
    Q_OBJECT
public:
    explicit Fifo(QObject *parent = 0);
    
    bool atEnd() const override;
    qint64 bytesAvailable() const override;
    bool isSequential() const override;

private:
    qint64 readData(char *data, qint64 len) override;
    qint64 writeData(const char *data, qint64 len) override;

    QByteArray _data;
};

#endif // FIFO_HPP
