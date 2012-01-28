#ifndef LOCKINGUI_HPP
#define LOCKINGUI_HPP

#include <QWidget>
#include "lockin2.hpp"

namespace Ui {
class LockinGui;
}

class Lockin2Gui : public QWidget
{
    Q_OBJECT
    
public:
    explicit Lockin2Gui(QWidget *parent = 0);
    ~Lockin2Gui();
        
private slots:
    void on_buttonStartStop_clicked();
    void getValues(qreal time, qreal x, qreal y);

private:
    QAudioFormat foundFormat(const QAudioDeviceInfo &device);

    template <typename T>
    T maxInList(const QList<T> &list, T def)
    {
        T max = def;
        for (int i = 0; i < list.size(); ++i)
            max = qMax(max, list[i]);
        return max;
    }

    Ui::LockinGui *ui;
    Lockin2 lockin;
};

#endif // LOCKINGUI_HPP
