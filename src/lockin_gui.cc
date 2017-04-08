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


#include "lockin_gui.hh"
#include "ui_lockin_gui.h"
#include "audioutils.hh"
#include "xy/xyscene.hh"
#include <QDebug>
#include <QTime>
#include <QSettings>
#include <QMessageBox>
#include <iostream>

LockinGui::LockinGui(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LockinGui)
{
    ui->setupUi(this);

    _lockin = new Lockin(this);

    foreach (const QAudioDeviceInfo &deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        QAudioFormat format = foundFormat(deviceInfo);

        if (deviceInfo.isFormatSupported(format) && _lockin->isFormatSupported(format)) {
            ui->audioDeviceSelector->addItem(deviceInfo.deviceName(), qVariantFromValue(deviceInfo));
        } else {
//            qDebug() << "deviceInfo =";
//            showQAudioDeviceInfo(deviceInfo);
//            format = deviceInfo.preferredFormat();
//            qDebug() << "format = deviceInfo.preferredFormat(); format =";
//            showQAudioFormat(format);
//            qDebug() << "deviceInfo.isFormatSupported(format) = " << deviceInfo.isFormatSupported(format);
//            qDebug() << " ";
        }
    }

    QSettings set;
    ui->outputFrequency->setValue(set.value("outputFrequency", 1.0 / _lockin->outputPeriod()).toDouble());
    ui->integrationTime->setValue(set.value("integrationTime", _lockin->integrationTime()).toDouble());
    _lockin->setPhase(set.value("phase", 0).toDouble());
    ui->vumeterTime->setValue(set.value("vumeterTime", 10).toInt());
    on_vumeterTime_valueChanged(ui->vumeterTime->value());

    connect(_lockin, SIGNAL(newVumeterData()), this, SLOT(updateGraphs()));
    connect(_lockin, SIGNAL(newValues(qreal,qreal,qreal)), this, SLOT(getValues(qreal,qreal,qreal)));

    _vumeter = new XYScene(this);
    _vumeter->setBackgroundBrush(QBrush(Qt::black));
    _vumeter->setZoom(0, 100, -1.1, 1.1);
    ui->vumeter->setScene(_vumeter);

    _vuScatterPlot = new XYPointList(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(Qt::green), 1.5));
    _vumeter->addScatterplot(_vuScatterPlot);


    _pll = new XYScene(this);
    _pll->setBackgroundBrush(QBrush(Qt::black));
    _pll->setZoom(0, 100, -1.1, 1.1);
    ui->pll->setScene(_pll);

    _pllScatterPlot = new XYPointList(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(Qt::green), 1.5));
    _pll->addScatterplot(_pllScatterPlot);


    _output = new XYScene(this);
    _output->setBackgroundBrush(QBrush(Qt::black));
    _output->setZoom(0.0, 15.0, -1.0, 1.0);
    ui->output->setScene(_output);

    _xScatterPlot = new XYPointList(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(Qt::red), 1.5));
    _output->addScatterplot(_xScatterPlot);
    _yScatterPlot = new XYPointList(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(Qt::darkGray), 1.5));
    _output->addScatterplot(_yScatterPlot);
}

LockinGui::~LockinGui()
{
    QSettings set;
    set.setValue("outputFrequency", ui->outputFrequency->value());
    set.setValue("integrationTime", ui->integrationTime->value());
    set.setValue("phase", _lockin->phase());
    set.setValue("vumeterTime", ui->vumeterTime->value());

    delete _vumeter;
    delete _pll;
    delete _output;

    delete _vuScatterPlot;
    delete _pllScatterPlot;
    delete _xScatterPlot;
    delete _yScatterPlot;
    //    qDebug() << __FUNCTION__ << ":" << __LINE__;

    delete ui;
}

void LockinGui::on_buttonStartStop_clicked()
{
    if (_lockin->isRunning()) {
        stopLockin();
    } else {
        startLockin();
    }
}

void LockinGui::updateGraphs()
{
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
}

void LockinGui::getValues(qreal time, qreal x, qreal y)
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
    str = str.arg(QTime(0, 0).addMSecs(1000 * time).toString());
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

//    qDebug() << __FUNCTION__ << ": execution time : " << execTime.elapsed() << " ms";
}

template <typename T>
T maxInList(const QList<T> &list, T def)
{
    T max = def;
    for (int i = 0; i < list.size(); ++i)
        max = qMax(max, list[i]);
    return max;
}

QAudioFormat LockinGui::foundFormat(const QAudioDeviceInfo &device)
{
    QAudioFormat format = device.preferredFormat();
    format.setChannelCount(2);
    format.setCodec("audio/pcm");

    format.setSampleSize(maxInList(device.supportedSampleSizes(), format.sampleSize()));

    if (!device.isFormatSupported(format)) {
        format = device.nearestFormat(format);

        // Required values
        format.setChannelCount(2);
        format.setCodec("audio/pcm");
    }

    return format;
}

void LockinGui::on_buttonAutoPhase_clicked()
{
    _lockin->setPhase(_lockin->autoPhase());
}

void LockinGui::startLockin()
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

void LockinGui::stopLockin()
{
    _lockin->stop();
    ui->frame->setEnabled(true);
    ui->buttonStartStop->setText("Start");
    ui->info->setText("lockin stoped");
}

void LockinGui::on_vumeterTime_valueChanged(int time_ms)
{
    _lockin->setVumeterTime(qreal(time_ms) / 1000.0);

    ui->vumeterLabel->setText(QString("%1 ms").arg(time_ms));
}
