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
#include "xygraph/xyscene.hh"
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

    foreach (const QAudioDeviceInfo &device, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        if (device.deviceName().contains("alsa_input")) {
            ui->audioDeviceSelector->addItem(device.deviceName(), qVariantFromValue(device));
        }
    }

    QSettings set;
    ui->outputFrequency->setValue(set.value("outputFrequency", 1.0 / _lockin->outputPeriod()).toDouble());
    ui->integrationTime->setValue(set.value("integrationTime", _lockin->integrationTime()).toDouble());
    _lockin->setPhase(set.value("phase", 0).toDouble());
    ui->dial->setValue(set.value("phase", 0).toDouble() * 10);

    connect(_lockin, SIGNAL(newRawData()), this, SLOT(updateGraphs()));
    connect(_lockin, SIGNAL(newValues(qreal,qreal,qreal)), this, SLOT(getValues(qreal,qreal,qreal)));

    _vumeter_left = new XYScene(this);
    _vumeter_left->setBackgroundBrush(QBrush(Qt::black));
    _vumeter_left->setAxesPen(QPen(Qt::lightGray));
    _vumeter_left->setSubaxesPen(QPen(QBrush(Qt::darkGray), 1, Qt::DashLine));
    _vumeter_left->setTextColor(Qt::gray);
    _vumeter_left->setZoom(0, 100, -1.1, 1.1);
    ui->vumeter->setScene(_vumeter_left);

    _vumeter_left_plot = new XYPointList(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(Qt::white), 1.5));
    _vumeter_left->addScatterplot(_vumeter_left_plot);


    _vumeter_right = new XYScene(this);
    _vumeter_right->setBackgroundBrush(QBrush(Qt::black));
    _vumeter_right->setAxesPen(QPen(Qt::lightGray));
    _vumeter_right->setSubaxesPen(QPen(QBrush(Qt::darkGray), 1, Qt::DashLine));
    _vumeter_right->setTextColor(Qt::gray);
    _vumeter_right->setZoom(0, 100, -1.1, 1.1);
    ui->pll->setScene(_vumeter_right);

    _vumeter_right_plot = new XYPointList(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(Qt::white), 1.5));
    _vumeter_right->addScatterplot(_vumeter_right_plot);
    _vumeter_sin_plot = new XYPointList(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(QColor(200, 200, 255)), 1.5, Qt::DashLine));
    _vumeter_right->addScatterplot(_vumeter_sin_plot);
    _vumeter_cos_plot = new XYPointList(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(QColor(200, 255, 200)), 1.5, Qt::DashLine));
    _vumeter_right->addScatterplot(_vumeter_cos_plot);


    _output = new XYScene(this);
    _output->setBackgroundBrush(QBrush(Qt::black));
    _output->setAxesPen(QPen(Qt::lightGray));
    _output->setSubaxesPen(QPen(QBrush(Qt::darkGray), 1, Qt::DashLine));
    _output->setTextColor(Qt::gray);
    _output->setZoom(0.0, 15.0, -1.0, 1.0);
    ui->output->setScene(_output);

    _y_plot = new XYPointList(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(Qt::darkGray), 1.5));
    _output->addScatterplot(_y_plot);
    _x_plot = new XYPointList(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(Qt::white), 1.5));
    _output->addScatterplot(_x_plot);

    _regraph_timer.setSingleShot(true);
    connect(&_regraph_timer, SIGNAL(timeout()), this, SLOT(regraph()));
}

LockinGui::~LockinGui()
{
    QSettings set;
    set.setValue("outputFrequency", ui->outputFrequency->value());
    set.setValue("integrationTime", ui->integrationTime->value());
    set.setValue("phase", _lockin->phase());

    delete _vumeter_left;
    delete _vumeter_right;
    delete _output;

    delete _vumeter_left_plot;
    delete _vumeter_right_plot;
    delete _x_plot;
    delete _y_plot;

    delete ui;
}

void LockinGui::on_dial_sliderMoved(int position)
{
    _lockin->setPhase(qreal(position) / 10.0);
}

void LockinGui::on_checkBox_clicked(bool checked)
{
    _lockin->setInvertLR(checked);
}

void LockinGui::on_audioDeviceSelector_currentIndexChanged(int arg1)
{
    QAudioDeviceInfo selected_device = ui->audioDeviceSelector->itemData(arg1).value<QAudioDeviceInfo>();

    ui->sampleRateComboBox->clear();
    foreach (int rate, selected_device.supportedSampleRates()) {
        ui->sampleRateComboBox->addItem(QString::number(rate), rate);
    }

    ui->sampleSizeComboBox->clear();
    foreach (int size, selected_device.supportedSampleSizes()) {
        ui->sampleSizeComboBox->addItem(QString::number(size), size);
    }
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
    const QVector<QPair<qreal, qreal>> &data = _lockin->raw_signals();
    const QVector<QPair<qreal, qreal>> &sin_cos = _lockin->sin_cos_signals();
    _vumeter_left_plot->clear();
    _vumeter_right_plot->clear();
    _vumeter_sin_plot->clear();
    _vumeter_cos_plot->clear();
    qreal msPerDot = 1000.0 / qreal(_lockin->format().sampleRate());
    for (int i = 0; i < qMin(data.size(), 2048); ++i) {
        qreal t = qreal(i) * msPerDot;
        _vumeter_left_plot->append(QPointF(t, data[i].first));
        _vumeter_right_plot->append(QPointF(t, data[i].second));
        if (!std::isnan(sin_cos[i].first)) {
            _vumeter_sin_plot->append(QPointF(t, sin_cos[i].first));
        }
        if (!std::isnan(sin_cos[i].second)) {
            _vumeter_cos_plot->append(QPointF(t, sin_cos[i].second));
        }
    }

    if (!_regraph_timer.isActive()) {
        _regraph_timer.start(50);
    }
}

void LockinGui::getValues(qreal time, qreal x, qreal y)
{
    ui->lcdForXValue->display(x);

    QString str = QString::fromUtf8("phase = %1°\n"
                                    "execution time = %2 (%3)\n"
                                    "sample rate = %4 Hz\n"
                                    "sample size = %5 bits");

    str = str.arg(_lockin->phase());
    str = str.arg(QTime(0, 0).addMSecs(1000 * time).toString());
    str = str.arg(QTime(0, 0).addMSecs(_run_time.elapsed()).toString());
    str = str.arg(_lockin->format().sampleRate());
    str = str.arg(_lockin->format().sampleSize());

	ui->info->setPlainText(str);

    std::cout << time << " " << x << std::endl;

    // output graph
    _x_plot->append(QPointF(time, x));
    _y_plot->append(QPointF(time, y));
    RealZoom zoom = _output->zoom();
    if (zoom.xMin() < time && zoom.xMax() < time && zoom.xMax() > time * 0.9)
        zoom.setXMax(time + 0.20 * zoom.width());
    _output->setZoom(zoom);
}

void LockinGui::regraph()
{
    _vumeter_left->regraph();
    _vumeter_right->regraph();
    _output->regraph();
}

void LockinGui::startLockin()
{
    QAudioDeviceInfo selected_device = ui->audioDeviceSelector->itemData(ui->audioDeviceSelector->currentIndex()).value<QAudioDeviceInfo>();

    qDebug() << "========== device infos ========== ";
    showQAudioDeviceInfo(selected_device);

    QAudioFormat format = selected_device.preferredFormat();
    format.setChannelCount(2);
    format.setCodec("audio/pcm");
    format.setSampleRate(ui->sampleRateComboBox->itemData(ui->sampleRateComboBox->currentIndex()).toInt());
    format.setSampleSize(ui->sampleSizeComboBox->itemData(ui->sampleSizeComboBox->currentIndex()).toInt());

    qDebug() << "========== format infos ========== ";
    qDebug() << format;

    _lockin->setOutputPeriod(1.0 / ui->outputFrequency->value());
    _lockin->setIntegrationTime(ui->integrationTime->value());

    if (_lockin->start(selected_device, format)) {
        _run_time.start();

        _x_plot->clear();
        _y_plot->clear();
        _vumeter_left_plot->clear();
        _vumeter_right_plot->clear();

        ui->frame->setEnabled(false);
        ui->buttonStartStop->setText("Stop !");

        QString str = QString::fromUtf8("phase = %1°\n"
                                        "execution time = <just started>\n"
                                        "sample rate = %2 Hz\n"
                                        "sample size = %3 bits");

        str = str.arg(_lockin->phase());
        str = str.arg(_lockin->format().sampleRate());
        str = str.arg(_lockin->format().sampleSize());

		ui->info->setPlainText(str);
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
	ui->info->setPlainText("lockin stoped");
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

    //    format.setSampleRate(maxInList(device.supportedSampleRates(), format.sampleRate()));
    // If the sample rate is too high, the lockin takes late
    format.setSampleRate(11025);

    format.setSampleSize(maxInList(device.supportedSampleSizes(), format.sampleSize()));

    if (!device.isFormatSupported(format)) {
        format = device.nearestFormat(format);

        // Required values
        format.setChannelCount(2);
        format.setCodec("audio/pcm");
    }

    return format;
}
