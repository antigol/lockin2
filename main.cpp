#include <QApplication>

#if defined(LOCKIN2_LIBRARY)
#  include "lockin2gui.hpp"
#else
#  include <lockin2/lockin2gui.hpp>
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Lockin2Gui l;
    l.show();

    return a.exec();
}
