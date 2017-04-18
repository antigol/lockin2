#include <QApplication>
#include "lockin_gui.hh"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    LockinGui l;
    l.show();

    return a.exec();
}
