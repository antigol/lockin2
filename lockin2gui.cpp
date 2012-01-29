#include "lockin2gui.hpp"
#include "ui_lockingui.h"
#include "audioutils.hpp"
#include <xygraph/xyscene.hpp>
#include <QDebug>
#include <QTime>
#include <QSettings>
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
            showQAudioDeviceInfo(deviceInfo);
            showQAudioFormat(format);
            qDebug() << " ";
        }
    }

    QSettings set;
    ui->outputFrequency->setValue(set.value("outputFrequency", 1.0 / _lockin->outputPeriod()).toDouble());
    ui->integrationTime->setValue(set.value("integrationTime", _lockin->integrationTime()).toDouble());
    _lockin->setPhase(set.value("phase", 0).toDouble());

    connect(_lockin, SIGNAL(newValues(qreal,qreal,qreal)), this, SLOT(getValues(qreal,qreal,qreal)));

    _vumeter = new XYScene(this);
    ui->vumeter->setScene(_vumeter);

    _vuScatterPlot = new XYScatterplot(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(Qt::green), 1.5));
    _vumeter->addScatterplot(_vuScatterPlot);


    _pll = new XYScene(this);
    ui->pll->setScene(_pll);

    _pllScatterPlot = new XYScatterplot(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(Qt::green), 1.5));
    _pll->addScatterplot(_pllScatterPlot);


    _output = new XYScene(this);
    _output->setZoom(0.0, 15.0, -100.0, 100.0);
    ui->output->setScene(_output);

    _xScatterPlot = new XYScatterplot(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(Qt::red), 1.5));
    _output->addScatterplot(_xScatterPlot);
    _yScatterPlot = new XYScatterplot(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(Qt::black), 1.5));
    _output->addScatterplot(_yScatterPlot);

}

Lockin2Gui::~Lockin2Gui()
{
    QSettings set;
    set.setValue("outputFrequency", ui->outputFrequency->value());
    set.setValue("integrationTime", ui->integrationTime->value());
    set.setValue("phase", _lockin->phase());
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

    info = info.arg(qRound(y / x * 1000.0)/10.0);
    info = info.arg(QTime().addMSecs(1000*time).toString("h:m:s,zzz"));
    ui->info->setText(info);
    std::cout << time << " " << x << std::endl;

    _xScatterPlot->append(QPointF(time, x));
    _yScatterPlot->append(QPointF(time, y));
    RealZoom zoom = _output->zoom();
    if (zoom.xMin() < time && zoom.xMax() < time && zoom.xMax() > time * 0.8)
        zoom.setXMax(time * 1.20);
    _output->setZoom(zoom);

    QList<QPair<qreal, qreal> > data = _lockin->vumeterData();
    _vuScatterPlot->clear();
    _pllScatterPlot->clear();
    for (int i = 0; i < data.size(); ++i) {
        _vuScatterPlot->append(QPointF(i, data[i].first));
        _pllScatterPlot->append(QPointF(i, data[i].second));
    }

    _vumeter->autoZoom();
    _vumeter->relativeZoom(1.2);

    _pll->autoZoom();
    _pll->relativeZoom(1.2);

    _output->regraph();
    _pll->regraph();
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
    format.setChannelCount(2);
    format.setCodec("audio/pcm");
    format.setSampleType(QAudioFormat::UnSignedInt);

    format.setSampleRate(maxInList(device.supportedSampleRates(), 44100));
    format.setSampleSize(32);


    if (!device.isFormatSupported(format)) {
        format = device.nearestFormat(format);

        // ces réglages sont indispensables pour le lockin
        format.setChannelCount(2);
        format.setCodec("audio/pcm");
        format.setSampleType(QAudioFormat::UnSignedInt);
    }

    if (!device.isFormatSupported(format)) {
        // on essaye encore avec la pire merde
        format.setSampleRate(8000);
        format.setSampleSize(8);
    }

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
    _lockin->setVumeterTime(0.01);

    if (_lockin->start(deviceInfo, format)) {
        _xScatterPlot->clear();
        _yScatterPlot->clear();
        _vuScatterPlot->clear();
        _pllScatterPlot->clear();

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
