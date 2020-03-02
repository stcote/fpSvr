#include "FpWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    FpWindow w;
    w.show();

    return a.exec();
}
