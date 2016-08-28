#include <qapplication.h>
#include "splatterBoardManip.h"

int main( int argc, char **argv ) {
  QApplication a( argc, argv );

  splatterBoardManip paintwin;

  paintwin.resize( 500, 500 );
  paintwin.setCaption("SplatterBoardManip");
  a.setMainWidget( &paintwin );
  paintwin.show();
  return a.exec();
}
