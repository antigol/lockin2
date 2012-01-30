#include "lockin2gui.hpp"
#include "ui_lockingui.h"
#include "audioutils.hpp"
#include <xygraph/xyscene.hpp>
#include <QDebug>
#include <QTime>
#include <QSettings>
#include <QMessageBox>
#include <iostream>

Lockin2Gui::Lockin2Gui(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LockinGui)
{
    ui->setupUi(this);

    _lockin = new Lockin2(this);

    foreach (const QAudioDeviceInfo &deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
//        QAudioFormat format = foundFormat(deviceInfo);

//        if (deviceInfo.isFormatSupported(format) && _lockin->isFormatSupported(format)) {
            ui->audioDeviceSelector->addItem(deviceInfo.deviceName(), qVariantFromValue(deviceInfo));
//            qDebug() << deviceInfo.deviceName() << " added in the list";
//        } else {
//            qDebug() << deviceInfo.deviceName() << " is not supported :";
//            qDebug() << "deviceInfo =";
//            showQAudioDeviceInfo(deviceInfo);
//            format = deviceInfo.preferredFormat();
//            qDebug() << "format = deviceInfo.preferredFormat(); format =";
//            showQAudioFormat(format);
//            qDebug() << "deviceInfo.isFormatSupported(format) = " << deviceInfo.isFormatSupported(format);
//            qDebug() << " ";
//        }
    }

    QSettings set;
    ui->outputFrequency->setValue(set.value("outputFrequency", 1.0 / _lockin->outputPeriod()).toDouble());
    ui->integrationTime->setValue(set.value("integrationTime", _lockin->integrationTime()).toDouble());
    _lockin->setPhase(set.value("phase", 0).toDouble());
    ui->vumeterTime->setValue(set.value("vumeterTime", 10).toInt());
    on_vumeterTime_valueChanged(ui->vumeterTime->value());

    connect(_lockin, SIGNAL(newValues(qreal,qreal,qreal)), this, SLOT(getValues(qreal,qreal,qreal)));

    _vumeter = new XYScene(this);
    _vumeter->setBackgroundBrush(QBrush(Qt::black));
    _vumeter->setZoom(0, 100, -1.1, 1.1);
    ui->vumeter->setScene(_vumeter);

    _vuScatterPlot = new XYScatterplot(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(Qt::green), 1.5));
    _vumeter->addScatterplot(_vuScatterPlot);


    _pll = new XYScene(this);
    _pll->setBackgroundBrush(QBrush(Qt::black));
    _pll->setZoom(0, 100, -1.1, 1.1);
    ui->pll->setScene(_pll);

    _pllScatterPlot = new XYScatterplot(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(Qt::green), 1.5));
    _pll->addScatterplot(_pllScatterPlot);


    _output = new XYScene(this);
    _output->setBackgroundBrush(QBrush(Qt::black));
    _output->setZoom(0.0, 15.0, -1.0, 1.0);
    ui->output->setScene(_output);

    _xScatterPlot = new XYScatterplot(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(Qt::red), 1.5));
    _output->addScatterplot(_xScatterPlot);
    _yScatterPlot = new XYScatterplot(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(Qt::darkGray), 1.5));
    _output->addScatterplot(_yScatterPlot);

}

Lockin2Gui::~Lockin2Gui()
{
    QSettings set;
    set.setValue("outputFrequency", ui->outputFrequency->value());
    set.setValue("integrationTime", ui->integrationTime->value());
    set.setValue("phase", _lockin->phase());
    set.setValue("vumeterTime", ui->vumeterTime->value());

    qDebug() << __FUNCTION__ << ":" << __LINE__;
    delete _vumeter;
    qDebug() << __FUNCTION__ << ":" << __LINE__;
    delete _pll;
    qDebug() << __FUNCTION__ << ":" << __LINE__;
    delete _output;
    qDebug() << __FUNCTION__ << ":" << __LINE__;

    delete _vuScatterPlot;
    qDebug() << __FUNCTION__ << ":" << __LINE__;
    delete _pllScatterPlot;
    qDebug() << __FUNCTION__ << ":" << __LINE__;
    delete _xScatterPlot;
    qDebug() << __FUNCTION__ << ":" << __LINE__;
    delete _yScatterPlot;
    qDebug() << __FUNCTION__ << ":" << __LINE__;

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
    QTime execTime;
    execTime.start();

    ui->lcdForXValue->display(x);

    QString str = QString::fromUtf8("phase = %1°\n"
                                    "autophase = %2°\n"
                                    "execution time = %3\n"
                                    "sample rate = %4 Hz\n"
                                    "sample size = %5 bits");

    str = str.arg(_lockin->phase());
    qreal d = _lockin->autoPhase() - _lockin->phase();
    str = str.arg((d>0 ? "+":"")+QString::number(d));
    str = str.arg(QTime().addMSecs(1000*time).toString("h:m:s,zzz"));
    str = str.arg(_lockin->format().sampleRate());
    str = str.arg(_lockin->format().sampleSize());

    ui->info->setText(str);

    std::cout << time << " " << x << std::endl;

    // output graph
    _xScatterPlot->append(QPointF(time, x));
    _yScatterPlot->append(QPointF(time, y));
    RealZoom zoom = _output->zoom();
    if (zoom.xMin() < time && zoom.xMax() < time && zoom.xMax() > time * 0.9)
        zoom.setXMax(time * 1.20);
    _output->setZoom(zoom);

    _output->regraph();

    // vumeter & pll graphs
    QList<QPair<qreal, qreal> > data = _lockin->vumeterData();
    _vuScatterPlot->clear();
    _pllScatterPlot->clear();
    qreal msPerDot = 1000.0 / qreal(_lockin->format().sampleRate());
    for (int i = 0; i < data.size(); ++i) {
        qreal t = i*msPerDot;
        _vuScatterPlot->append(QPointF(t, data[i].first));
        _pllScatterPlot->append(QPointF(t, data[i].second));
    }

    _vumeter->setXMin(0.0);
    _vumeter->setXMax(msPerDot*data.size());

    _vumeter->regraph();

    _pll->setXMin(0.0);
    _pll->setXMax(msPerDot*data.size());

    _pll->regraph();

    qDebug() << __FUNCTION__ << ": execution time : " << execTime.elapsed() << " ms";
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

    format.setSampleRate(maxInList(device.supportedSampleRates(), 44100));
    format.setSampleSize(32);


    if (!device.isFormatSupported(format)) {
        format = device.nearestFormat(format);

        // ces réglages sont indispensables pour le lockin
        format.setChannelCount(2);
        format.setCodec("audio/pcm");
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

    if (_lockin->start(deviceInfo, format)) {
        _xScatterPlot->clear();
        _yScatterPlot->clear();
        _vuScatterPlot->clear();
        _pllScatterPlot->clear();

        ui->frame->setEnabled(false);
        ui->buttonStartStop->setText("Stop !");

        QString str = QString::fromUtf8("phase = %1°\n"
                                        "sample rate = %2 Hz\n"
                                        "sample size = %3 bits");

        str = str.arg(_lockin->phase());
        str = str.arg(_lockin->format().sampleRate());
        str = str.arg(_lockin->format().sampleSize());

        ui->info->setText(str);

    } else {
        qDebug() << __FUNCTION__ << ": cannot start lockin";
        QMessageBox::warning(this, "Start lockin fail", "Start has failed, maybe retry can work.");
    }
}

void Lockin2Gui::stopLockin()
{
    _lockin->stop();
    ui->frame->setEnabled(true);
    ui->buttonStartStop->setText("Start");
    ui->info->setText("lockin stoped");
}

void Lockin2Gui::on_vumeterTime_valueChanged(int timems)
{
    _lockin->setVumeterTime(qreal(timems) / 1000.0);

    ui->vumeterLabel->setText(QString("%1 ms").arg(timems));
}
