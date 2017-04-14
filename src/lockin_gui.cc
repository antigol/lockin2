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
    ui->outputPeriod->setValue(set.value("output period", _lockin->outputPeriod()).toDouble());
    ui->integrationTime->setValue(set.value("integration time", _lockin->integrationTime()).toDouble());

    connect(_lockin, SIGNAL(newRawData()), this, SLOT(updateGraphs()));
    connect(_lockin, SIGNAL(newValue(qreal,qreal)), this, SLOT(getValue(qreal,qreal)));

    _vumeter_left = new XYScene(this);
    _vumeter_left->setBackgroundBrush(QBrush(Qt::black));
    _vumeter_left->setAxesPen(QPen(Qt::lightGray));
    _vumeter_left->setSubaxesPen(QPen(QBrush(Qt::darkGray), 1, Qt::DashLine));
    _vumeter_left->setTextColor(Qt::gray);
    _vumeter_left->setZoom(0, 100, -1.1, 1.1);
    ui->left->setScene(_vumeter_left);

    _vumeter_left_plot = new XYPointList(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(Qt::white), 1.5));
    _vumeter_left->addScatterplot(_vumeter_left_plot);


    _vumeter_right = new XYScene(this);
    _vumeter_right->setBackgroundBrush(QBrush(Qt::black));
    _vumeter_right->setAxesPen(QPen(Qt::lightGray));
    _vumeter_right->setSubaxesPen(QPen(QBrush(Qt::darkGray), 1, Qt::DashLine));
    _vumeter_right->setTextColor(Qt::gray);
    _vumeter_right->setZoom(0, 100, -1.1, 1.1);
    ui->right->setScene(_vumeter_right);

    _vumeter_right_plot = new XYPointList(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(Qt::white), 1.5));
    _vumeter_sin_plot = new XYPointList(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(Qt::gray), 1., Qt::DashLine));
    _vumeter_right->addScatterplot(_vumeter_sin_plot);
    _vumeter_right->addScatterplot(_vumeter_right_plot);


    _output = new XYScene(this);
    _output->setBackgroundBrush(QBrush(Qt::black));
    _output->setAxesPen(QPen(Qt::lightGray));
    _output->setSubaxesPen(QPen(QBrush(Qt::darkGray), 1, Qt::DashLine));
    _output->setTextColor(Qt::gray);
    _output->setZoom(0.0, 15.0, -1.0, 1.0);
    ui->output->setScene(_output);

    _measures_plot = new XYPointList(QPen(Qt::NoPen), QBrush(Qt::NoBrush), 0.0, QPen(QBrush(Qt::white), 1.5));
    _output->addScatterplot(_measures_plot);

    _regraph_timer.setSingleShot(true);
    connect(&_regraph_timer, SIGNAL(timeout()), this, SLOT(regraph()));
}

LockinGui::~LockinGui()
{
    QSettings set;
    set.setValue("output period", ui->outputPeriod->value());
    set.setValue("integration time", ui->integrationTime->value());

    delete _vumeter_left;
    delete _vumeter_right;
    delete _output;

    delete _vumeter_left_plot;
    delete _vumeter_right_plot;
    delete _measures_plot;

    delete ui;
}

const QList<QPointF> &LockinGui::values() const
{
    return *_measures_plot;
}

const QTime &LockinGui::start_time() const
{
    return _start_time;
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
    ui->sampleSizeComboBox->setCurrentIndex(ui->sampleSizeComboBox->count() - 1);
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
    const QVector<std::complex<qreal>> &sin_cos = _lockin->complex_exp_signal();
    _vumeter_left_plot->clear();
    _vumeter_right_plot->clear();
    _vumeter_sin_plot->clear();
    qreal msPerDot = 1000.0 / qreal(_lockin->format().sampleRate());

    int trigger = 0;
    for (int i = 0; i < qMin(data.size(), 2048); ++i) {
        if (!std::isnan(sin_cos[i].real()) && !std::isnan(sin_cos[i].imag())) {
            trigger = i;
            break;
        }
    }

    for (int i = 0; i < qMin(data.size(), 2048); ++i) {
        qreal t = qreal(i - trigger) * msPerDot;
        _vumeter_left_plot->append(QPointF(t, data[i].first));
        _vumeter_right_plot->append(QPointF(t, data[i].second));
        if (!std::isnan(sin_cos[i].real())) {
            _vumeter_sin_plot->append(QPointF(t, sin_cos[i].imag()));
        }
    }

    if (!_regraph_timer.isActive()) {
        _regraph_timer.start(50);
    }
}

void LockinGui::getValue(qreal time, qreal measure)
{
    ui->label_current_value->setText(QString::number(measure));
    ui->label_current_time->setText(QTime(0, 0).addMSecs(1000 * time).toString());
    ui->label_real_time->setText(QTime(0, 0).addMSecs(_run_time.elapsed()).toString());

    // output graph
    _measures_plot->append(QPointF(time, measure));

    RealZoom zoom = _output->zoom();
    if (zoom.xMin() < time && zoom.xMax() < time && zoom.xMax() > time * 0.9)
        zoom.setXMax(time + 0.20 * zoom.width());
    _output->setZoom(zoom);

    emit newValue();
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

//    qDebug() << "========== device infos ========== ";
//    showQAudioDeviceInfo(selected_device);

    QAudioFormat format = selected_device.preferredFormat();
    format.setChannelCount(2);
    format.setCodec("audio/pcm");
    format.setSampleRate(ui->sampleRateComboBox->itemData(ui->sampleRateComboBox->currentIndex()).toInt());
    format.setSampleSize(ui->sampleSizeComboBox->itemData(ui->sampleSizeComboBox->currentIndex()).toInt());

//    qDebug() << "========== format infos ========== ";
    qDebug() << format;

    _lockin->setOutputPeriod(ui->outputPeriod->value());
    _lockin->setIntegrationTime(ui->integrationTime->value());

    if (_lockin->start(selected_device, format)) {
        _run_time.start();
        _start_time = QTime::currentTime();

        _measures_plot->clear();
        _vumeter_left_plot->clear();
        _vumeter_right_plot->clear();

        ui->frame->setEnabled(false);
        ui->buttonStartStop->setText("Stop !");
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
}

template <typename T>
T maxInList(const QList<T> &list, T def)
{
    T max = def;
    for (int i = 0; i < list.size(); ++i)
        max = qMax(max, list[i]);
    return max;
}



