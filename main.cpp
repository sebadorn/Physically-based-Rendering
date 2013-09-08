#include <QApplication>
#include <QDesktopWidget>

#include "source/qt/Window.h"


int main( int argc, char* argv[] ) {
	setlocale( LC_ALL, "C" );

	QApplication app( argc, argv );
	Window window;
	window.resize( window.sizeHint() );

	float desktopArea = QApplication::desktop()->width() * QApplication::desktop()->height();
	float widgetArea = window.width() * window.height();

	if( widgetArea / desktopArea < 0.75f ) {
		window.show();
	}
	else {
		window.showMaximized();
	}

	return app.exec();
}
