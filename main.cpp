#include "room_window.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    room_window w;
    w.show();

    return a.exec();
}
