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

#ifndef LOCKINGUI_HPP
#define LOCKINGUI_HPP

#include <QAudioDeviceInfo>
#include <QWidget>
#include <QTime>
#include <QTimer>
#include "lockin.hh"
#include "xygraph/xyscene.hh"
#include "xygraph/xyview.hh"

namespace Ui {
class LockinGui;
}

class LockinGui : public QWidget
{
    Q_OBJECT

public:
    explicit LockinGui(QWidget *parent = 0);
    ~LockinGui();

private slots:
    void on_buttonStartStop_clicked();
    void on_buttonAutoPhase_clicked();
    void updateGraphs();
    void getValues(qreal time, qreal x, qreal y);
    void regraph();

private:
    void startLockin();
    void stopLockin();
    QAudioFormat foundFormat(const QAudioDeviceInfo &device);

    Ui::LockinGui *ui;

    Lockin *_lockin;
    QTime _run_time;
    QTimer _regraph_timer;

    // Plots
    XYScene *_vumeter_left;
    XYPointList *_vumeter_left_plot;

    XYScene *_vumeter_right;
    XYPointList *_vumeter_right_plot;

    XYScene *_output;
    XYPointList *_x_plot;
    XYPointList *_y_plot;
};

#endif // LOCKINGUI_HPP
