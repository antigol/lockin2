#ifndef LOCKINGUI_HPP
#define LOCKINGUI_HPP

#include "lockin2_global.hpp"
#include "lockin2.hpp"

#include <QWidget>
#include <xygraph/xygraph.hpp>
#include <xygraph/xyscene.hpp>

namespace Ui {
class LockinGui;
}

class LOCKIN2SHARED_EXPORT Lockin2Gui : public QWidget
{
    Q_OBJECT
    
public:
    explicit Lockin2Gui(QWidget *parent = 0);
    ~Lockin2Gui();
        
private slots:
    void on_buttonStartStop_clicked();
    void getValues(qreal time, qreal x, qreal y);
    void on_buttonAutoPhase_clicked();
    void on_vumeterTime_valueChanged(int timems);

private:
    void startLockin();
    void stopLockin();
    QAudioFormat foundFormat(const QAudioDeviceInfo &device);

    Ui::LockinGui *ui;
    Lockin2 *_lockin;

    XYScene *_vumeter;
    XYScatterplot *_vuScatterPlot;

    XYScene *_pll;
    XYScatterplot *_pllScatterPlot;

    XYScene *_output;
    XYScatterplot *_xScatterPlot;
    XYScatterplot *_yScatterPlot;
};

#endif // LOCKINGUI_HPP
