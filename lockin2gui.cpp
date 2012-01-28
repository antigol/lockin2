#include "lockin2gui.hpp"
#include "ui_lockingui.h"
#include "audioutils.hpp"
#include <QDebug>
#include <QTime>
#include <iostream>

Lockin2Gui::Lockin2Gui(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LockinGui)
{
    ui->setupUi(this);

    foreach (const QAudioDeviceInfo &deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        QAudioFormat format = foundFormat(deviceInfo);

        if (deviceInfo.isFormatSupported(format) && lockin.isFormatSupported(format)) {
            ui->audioDeviceSelector->addItem(deviceInfo.deviceName(), qVariantFromValue(deviceInfo));
            qDebug() << deviceInfo.deviceName() << " added in the list";
        } else {
            qDebug() << deviceInfo.deviceName() << " is not supported";
        }
    }

    ui->outputFrequency->setValue(1.0 / lockin.outputPeriod());
    ui->integrationTime->setValue(lockin.integrationTime());

    connect(&lockin, SIGNAL(newValues(qreal,qreal,qreal)), this, SLOT(getValues(qreal,qreal,qreal)));
}

Lockin2Gui::~Lockin2Gui()
{
    delete ui;
}

void Lockin2Gui::on_buttonStartStop_clicked()
{
    if (lockin.isRunning()) {
        stopLockin();
    } else {
        startLockin();
    }
}

void Lockin2Gui::getValues(qreal time, qreal x, qreal y)
{
    ui->lcdForXValue->display(x);

    QString info("y/x = %1%\n"
                 "execution time = %2");

    info = info.arg(y / x * 100.0);
    info = info.arg(QTime().addMSecs(1000*time).toString("h:m:s,zzz"));
    ui->info->setText(info);

    std::cout << time << " " << x << std::endl;
}

template <typename T>
T maxInList(const QList<T> &list, T def)
{
    T max = def;
    for (int i = 0; i < list.size(); ++i)
        max = qMax(max, list[i]);
    return max;
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
    format.setSampleType(QAudioFormat::UnSignedInt);

    return format;
}

void Lockin2Gui::on_buttonAutoPhase_clicked()
{
    lockin.setPhase(lockin.autoPhase());
}

void Lockin2Gui::startLockin()
{
    QAudioDeviceInfo deviceInfo = ui->audioDeviceSelector->itemData(ui->audioDeviceSelector->currentIndex()).value<QAudioDeviceInfo>();

    qDebug() << "========== device infos ========== ";
    showQAudioDeviceInfo(deviceInfo);

    QAudioFormat format = foundFormat(deviceInfo);

    qDebug() << "========== format infos ========== ";
    showQAudioFormat(format);

    lockin.setOutputPeriod(1.0 / ui->outputFrequency->value());
    lockin.setIntegrationTime(ui->integrationTime->value());

    if (lockin.start(deviceInfo, format)) {
        ui->frame->setEnabled(false);
        ui->buttonStartStop->setText("Stop !");
    } else {
        qDebug() << __FUNCTION__ << ": cannot start lockin";
    }
}

void Lockin2Gui::stopLockin()
{
    lockin.stop();
    ui->frame->setEnabled(true);
    ui->buttonStartStop->setText("Start");
}
