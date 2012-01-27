#include "lockin2.hpp"

Lockin2::Lockin2(QObject *parent) :
    QObject(parent)
{
    _input.start(&_mainBuffer);
}
