#ifndef LOCKIN2_HPP
#define LOCKIN2_HPP

#include "lockin2_global.hpp"

#include <QObject>
#include <QAudioInput>
#include <QAudioFormat>
#include <QBuffer>

class LOCKIN2SHARED_EXPORT Lockin2 : public QObject {
    Q_OBJECT
public:
    explicit Lockin2(QObject *parent = 0);

private:
    QAudioInput _input;
    QBuffer _mainBuffer;
};

#endif // LOCKIN2_HPP
