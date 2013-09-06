#ifndef WINDOW_H
#define WINDOW_H

#include <QtGui>
#include <QWidget>


class GLWidget;


class Window : public QWidget {

	Q_OBJECT

	public:
		Window();
		void updateStatus( const char *msg );

	protected:
		void keyPressEvent( QKeyEvent *e );
		void mouseMoveEvent( QMouseEvent *e );
		void mousePressEvent( QMouseEvent *e );

	private:
		int mMouseLastX;
		int mMouseLastY;
		GLWidget *mGlWidget;
		QStatusBar *mStatusBar;

};

#endif
