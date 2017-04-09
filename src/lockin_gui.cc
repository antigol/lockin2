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

    _vumeter_left = new XYScene(this);
    _vumeter_left->setBackgroundBrush(QBrush(Qt::black));
    _vumeter_left->setZoom(0, 100, -1.1, 1.1);
    ui->vumeter->setScene(_vumeter_left);

    _vumeter_left_plot = new XYPointList(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(Qt::green), 1.5));
    _vumeter_left->addScatterplot(_vumeter_left_plot);


    _vumeter_right = new XYScene(this);
    _vumeter_right->setBackgroundBrush(QBrush(Qt::black));
    _vumeter_right->setZoom(0, 100, -1.1, 1.1);
    ui->pll->setScene(_vumeter_right);

    _vumeter_right_plot = new XYPointList(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(Qt::green), 1.5));
    _vumeter_right->addScatterplot(_vumeter_right_plot);


    _output = new XYScene(this);
    _output->setBackgroundBrush(QBrush(Qt::black));
    _output->setZoom(0.0, 15.0, -1.0, 1.0);
    ui->output->setScene(_output);

    _x_plot = new XYPointList(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(Qt::red), 1.5));
    _output->addScatterplot(_x_plot);
    _y_plot = new XYPointList(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(Qt::darkGray), 1.5));
    _output->addScatterplot(_y_plot);
}

LockinGui::~LockinGui()
{
    QSettings set;
    set.setValue("outputFrequency", ui->outputFrequency->value());
    set.setValue("integrationTime", ui->integrationTime->value());
    set.setValue("phase", _lockin->phase());
    set.setValue("vumeterTime", ui->vumeterTime->value());

    delete _vumeter_left;
    delete _vumeter_right;
    delete _output;

    delete _vumeter_left_plot;
    delete _vumeter_right_plot;
    delete _x_plot;
    delete _y_plot;
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

void LockinGui::on_buttonAutoPhase_clicked()
{
    _lockin->setPhase(_lockin->autoPhase());
}

void LockinGui::on_vumeterTime_valueChanged(int time_ms)
{
    _lockin->setVumeterTime(qreal(time_ms) / 1000.0);

    ui->vumeterLabel->setText(QString("%1 ms").arg(time_ms));
}

void LockinGui::updateGraphs()
{
    const QVector<QPair<qreal, qreal>> &data = _lockin->vumeterData();
    _vumeter_left_plot->clear();
    _vumeter_right_plot->clear();
    qreal msPerDot = 1000.0 / qreal(_lockin->format().sampleRate());
    for (int i = 0; i < data.size(); ++i) {
        qreal t = i*msPerDot;
        _vumeter_left_plot->append(QPointF(t, data[i].first));
        _vumeter_right_plot->append(QPointF(t, data[i].second));
    }

    _vumeter_left->setXMin(0.0);
    _vumeter_left->setXMax(msPerDot*data.size());

    _vumeter_left->regraph();

    _vumeter_right->setXMin(0.0);
    _vumeter_right->setXMax(msPerDot*data.size());

    _vumeter_right->regraph();
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
    _x_plot->append(QPointF(time, x));
    _y_plot->append(QPointF(time, y));
    RealZoom zoom = _output->zoom();
    if (zoom.xMin() < time && zoom.xMax() < time && zoom.xMax() > time * 0.9)
        zoom.setXMax(time + 0.20 * zoom.width());
    _output->setZoom(zoom);

    _output->regraph();

//    qDebug() << __FUNCTION__ << ": execution time : " << execTime.elapsed() << " ms";
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
        _x_plot->clear();
        _y_plot->clear();
        _vumeter_left_plot->clear();
        _vumeter_right_plot->clear();

        ui->frame->setEnabled(false);
        ui->buttonStartStop->setText("Stop !");

        QString str = QString::fromUtf8("phase = %1°\n"
                                        "sample rate = %2 Hz\n"
                                        "sample size = %3 bits");

        str = str.arg(_lockin->phase());
        str = str.arg(_lockin->format().sampleRate());
        str = str.arg(_lockin->format().sampleSize());

        ui->info->setText(str);

        ui->buttonAutoPhase->setEnabled(true);

    } else {
        qDebug() << __FUNCTION__ << ": cannot start lockin";
        QMessageBox::warning(this, "Start lockin fail", "Start has failed.");
    }
}

void LockinGui::stopLockin()
{
    _lockin->stop();
    ui->frame->setEnabled(true);
    ui->buttonStartStop->setText("Start");
    ui->info->setText("lockin stoped");
    ui->buttonAutoPhase->setEnabled(false);
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

