#include <QApplication>
#include "lockin2gui.hh"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Lockin2Gui l;
    l.show();

    return a.exec();
}
