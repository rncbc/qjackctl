#include <qapplication.h>
#include "qjackctlMainForm.h"

int main( int argc, char ** argv )
{
    QApplication a( argc, argv );
    qjackctlMainForm w;
    w.show();
    a.connect( &a, SIGNAL( lastWindowClosed() ), &a, SLOT( quit() ) );
    return a.exec();
}
