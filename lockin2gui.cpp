#include "lockin2gui.hpp"
#include "ui_lockingui.h"
#include "audioutils.hpp"
#include <xygraph/xyscene.hpp>
#include <QDebug>
#include <QTime>
#include <iostream>

Lockin2Gui::Lockin2Gui(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LockinGui)
{
    ui->setupUi(this);

    _lockin = new Lockin2(this);

    foreach (const QAudioDeviceInfo &deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        QAudioFormat format = foundFormat(deviceInfo);

        if (deviceInfo.isFormatSupported(format) && _lockin->isFormatSupported(format)) {
            ui->audioDeviceSelector->addItem(deviceInfo.deviceName(), qVariantFromValue(deviceInfo));
            qDebug() << deviceInfo.deviceName() << " added in the list";
        } else {
            qDebug() << deviceInfo.deviceName() << " is not supported";
        }
    }

    ui->outputFrequency->setValue(1.0 / _lockin->outputPeriod());
    ui->integrationTime->setValue(_lockin->integrationTime());

    connect(_lockin, SIGNAL(newValues(qreal,qreal,qreal)), this, SLOT(getValues(qreal,qreal,qreal)));

    _vumeter = new XYScene();
    _xScatterPlot = new XYScatterplot(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(Qt::red));
    _yScatterPlot = new XYScatterplot(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(Qt::lightGray));
    _vumeter->addScatterplot(_xScatterPlot);
    _vumeter->addScatterplot(_yScatterPlot);
    _vumeter->setZoom(0.0, 15.0, 0.0, 100.0);
    ui->vumeter->setScene(_vumeter);
}

Lockin2Gui::~Lockin2Gui()
{
    delete ui;
}

void Lockin2Gui::on_buttonStartStop_clicked()
{
    if (_lockin->isRunning()) {
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

    _xScatterPlot->append(QPointF(time, x));
    _yScatterPlot->append(QPointF(time, y));
    RealZoom zoom = _vumeter->zoom();
    if (zoom.xMin() < time && zoom.xMax() < time && zoom.xMax() > time * 0.8)
        zoom.setXMax(time * 1.05);

    _vumeter->setZoom(zoom);
    _vumeter->regraph();
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
    _lockin->setPhase(_lockin->autoPhase());
}

void Lockin2Gui::startLockin()
{
    QAudioDeviceInfo deviceInfo = ui->audioDeviceSelector->itemData(ui->audioDeviceSelector->currentIndex()).value<QAudioDeviceInfo>();

    qDebug() << "========== device infos ========== ";
    showQAudioDeviceInfo(deviceInfo);

    QAudioFormat format = foundFormat(deviceInfo);

    qDebug() << "========== format infos ========== ";
    showQAudioFormat(format);

    _lockin->setOutputPeriod(1.0 / ui->outputFrequency->value());
    _lockin->setIntegrationTime(ui->integrationTime->value());

    if (_lockin->start(deviceInfo, format)) {
        _xScatterPlot->clear();
        ui->frame->setEnabled(false);
        ui->buttonStartStop->setText("Stop !");
    } else {
        qDebug() << __FUNCTION__ << ": cannot start lockin";
    }
}

void Lockin2Gui::stopLockin()
{
    _lockin->stop();
    ui->frame->setEnabled(true);
    ui->buttonStartStop->setText("Start");
}
