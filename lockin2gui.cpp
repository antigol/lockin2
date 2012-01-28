#include "lockin2gui.hpp"
#include "ui_lockingui.h"
#include <QDebug>
#include "qaudioutils.hpp"

Lockin2Gui::Lockin2Gui(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LockinGui)
{
    ui->setupUi(this);

    foreach (const QAudioDeviceInfo &deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        QAudioFormat format = foundFormat(deviceInfo);

        qDebug() << "===============================================================";

        showQAudioDeviceInfo(deviceInfo);
        qDebug() << "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -";
        showQAudioFormat(format);

        if (deviceInfo.isFormatSupported(format) && lockin.isFormatSupported(format))
            ui->audioDeviceSelector->addItem(deviceInfo.deviceName(), qVariantFromValue(deviceInfo));
         else
            qDebug() << deviceInfo.deviceName() << " not supported";
    }

    connect(&lockin, SIGNAL(newValues(qreal,qreal,qreal)), this, SLOT(getValues(qreal,qreal,qreal)));
}

Lockin2Gui::~Lockin2Gui()
{
    delete ui;
}

void Lockin2Gui::on_buttonStartStop_clicked()
{
    QAudioDeviceInfo deviceInfo = ui->audioDeviceSelector->itemData(ui->audioDeviceSelector->currentIndex()).value<QAudioDeviceInfo>();

    qDebug() << "========== device ========== ";
    showQAudioDeviceInfo(deviceInfo);

    QAudioFormat format = foundFormat(deviceInfo);

    qDebug() << "========== format ========== ";
    showQAudioFormat(format);

    if (lockin.start(deviceInfo, format)) {
        ui->audioDeviceSelector->setEnabled(false);
        qDebug() << "========== ok ========== ";
    } else {
        qDebug() << "========== flop ========== ";
    }
}

void Lockin2Gui::getValues(qreal time, qreal x, qreal y)
{
    qDebug() << time;
    ui->lcdForXValue->display(x);
    ui->lcdForYValue->display(y);
}

QAudioFormat Lockin2Gui::foundFormat(const QAudioDeviceInfo &device)
{
    QAudioFormat format = device.preferredFormat();

    format.setSampleRate(maxInList(device.supportedSampleRates(), 44100));
    format.setChannelCount(2);
    format.setCodec("audio/pcm");
    format.setSampleSize(32);
    format.setSampleType(QAudioFormat::UnSignedInt);

    if (!device.isFormatSupported(format))
        format = device.nearestFormat(format);

    format.setChannelCount(2);
    format.setCodec("audio/pcm");
    format.setSampleSize(32);
    format.setSampleType(QAudioFormat::UnSignedInt);

    return format;
}
