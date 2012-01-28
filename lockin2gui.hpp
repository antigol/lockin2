#ifndef LOCKINGUI_HPP
#define LOCKINGUI_HPP

#include <QWidget>
#include "lockin2_global.hpp"

#if defined(LOCKIN2_LIBRARY)
#  include "lockin2.hpp"
#else
#  include <lockin2/lockin2.hpp>
#endif

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


private:
    void startLockin();
    void stopLockin();
    QAudioFormat foundFormat(const QAudioDeviceInfo &device);

    Ui::LockinGui *ui;
    Lockin2 lockin;
};

#endif // LOCKINGUI_HPP
