#ifndef WINDOW_H
#define WINDOW_H

#include <QtGui>
#include <string>

#include "../Cfg.h"
#include "../Logger.h"
#include "GLWidget.h"
#include "InfoWindow.h"


class GLWidget;


class Window : public QWidget {

	Q_OBJECT

	public:
		Window();
		void updateStatus( const char* msg );

	protected:
		QBoxLayout* createLayout();
		QMenuBar* createMenuBar();
		QStatusBar* createStatusBar();
		void keyPressEvent( QKeyEvent* e );
		void mouseMoveEvent( QMouseEvent* e );
		void mousePressEvent( QMouseEvent* e );
		void toggleFullscreen();

	protected slots:
		void importFile();
		void openInfo();

	private:
		int mMouseLastX;
		int mMouseLastY;
		GLWidget* mGLWidget;
		QMenuBar* mMenuBar;
		QStatusBar* mStatusBar;

};

#endif
