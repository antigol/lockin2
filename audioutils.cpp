#include "audioutils.hpp"
#include <QDebug>

void showQAudioFormat(const QAudioFormat &format)
{
    qDebug() << "codec : " << format.codec();
    qDebug() << "Sample Rate : " << format.sampleRate();
    qDebug() << "Number of channels : " << format.channelCount();
    qDebug() << "Sample size : " << format.sampleSize();
    switch (format.sampleType()) {
    case QAudioFormat::Unknown:
        qDebug() << "Sample type : Unknown";
        break;
    case QAudioFormat::SignedInt:
        qDebug() << "Sample type : SignedInt";
        break;
    case QAudioFormat::UnSignedInt:
        qDebug() << "Sample type : UnSignedInt";
        break;
    case QAudioFormat::Float:
        qDebug() << "Sample type : Float";
        break;
    }
    switch (format.byteOrder()) {
    case QAudioFormat::BigEndian:
        qDebug() << "Byte Order : BigEndian";
        break;
    case QAudioFormat::LittleEndian:
        qDebug() << "Byte Order : LittleEndian";
        break;
    }
}

QStringList byteOrdersToString(QList<QAudioFormat::Endian> list)
{
    QStringList ret;
    for (int i = 0; i < list.size(); ++i) {
        switch (list[i]) {
        case QAudioFormat::BigEndian:
            ret << "BigEndian";
            break;
        case QAudioFormat::LittleEndian:
            ret << "LittleEndian";
            break;
        }
    }
    return ret;
}

QStringList sampleTypesToString(QList<QAudioFormat::SampleType> list)
{
    QStringList ret;
    for (int i = 0 ; i < list.size(); ++i) {

        switch (list[i]) {
        case QAudioFormat::Unknown:
            ret << "Unknown";
            break;
        case QAudioFormat::SignedInt:
            ret << "SignedInt";
            break;
        case QAudioFormat::UnSignedInt:
            ret << "UnSignedInt";
            break;
        case QAudioFormat::Float:
            ret << "Float";
            break;
        }
    }
    return ret;
}

void showQAudioDeviceInfo(const QAudioDeviceInfo &device)
{
    qDebug() << "Device name : " << device.deviceName();
    qDebug() << "supported codecs : " << device.supportedCodecs();
    qDebug() << "supported sample rate : " << device.supportedSampleRates();
    qDebug() << "supported channel count : " << device.supportedChannelCounts();
    qDebug() << "supported sample size : " << device.supportedSampleSizes();
    qDebug() << "supported sample type : " << sampleTypesToString(device.supportedSampleTypes());
    qDebug() << "supported byte orders : " << byteOrdersToString(device.supportedByteOrders());

}
