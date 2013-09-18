#ifndef WINDOW_H
#define WINDOW_H

#include <QtGui>
#include <string>

#include "../Logger.h"
#include "GLWidget.h"


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

	protected slots:
		void importFile();

	private:
		int mMouseLastX;
		int mMouseLastY;
		GLWidget* mGLWidget;
		QStatusBar* mStatusBar;

};

#endif
