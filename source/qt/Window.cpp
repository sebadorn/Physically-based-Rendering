#include <iostream>
#include <cmath>
#include <QtGui>
#include <QSizePolicy>

#include "GLWidget.h"
#include "Window.h"

#define WINDOW_TITLE "Physically-based Renderer"


/**
 * Constructor.
 */
Window::Window() {
	setlocale( LC_ALL, "C" );

	mMouseLastX = 0;
	mMouseLastY = 0;

	mGlWidget = new GLWidget;
	mStatusBar = this->createStatusBar();

	this->setLayout( this->createLayout() );
	this->setWindowTitle( tr( WINDOW_TITLE ) );
}


/**
 * Create the main layout.
 * @return {QBoxLayout*} The main layout.
 */
QBoxLayout* Window::createLayout() {
	QVBoxLayout *mainLayout = new QVBoxLayout();
	mainLayout->setSpacing( 0 );
	mainLayout->setMargin( 0 );
	mainLayout->addWidget( this->createMenuBar() );
	mainLayout->addWidget( mGlWidget );
	mainLayout->addWidget( mStatusBar );

	return mainLayout;
}


/**
 * Create the menu bar.
 * @return {QMenuBar*} The menu bar.
 */
QMenuBar* Window::createMenuBar() {
	QAction *actionExit = new QAction( tr( "&Exit" ), this );
	actionExit->setShortcuts( QKeySequence::Quit );
	actionExit->setStatusTip( tr( "Quit the application." ) );
	connect( actionExit, SIGNAL( triggered() ), this, SLOT( close() ) );

	QMenu *menuFile = new QMenu( tr( "&File" ) );
	menuFile->addAction( actionExit );

	QMenuBar *menubar = new QMenuBar( this );
	menubar->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
	menubar->addMenu( menuFile );

	return menubar;
}


/**
 * Create the status bar.
 * @return {QStatusBar*} The status bar.
 */
QStatusBar* Window::createStatusBar() {
	QStatusBar *statusBar = new QStatusBar( this );
	statusBar->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
	statusBar->showMessage( tr( "0 FPS" ) );

	return statusBar;
}


/**
 * Handle key press events.
 * @param e {QKeyEvent*} e Key event triggered by pressing a key.
 */
void Window::keyPressEvent( QKeyEvent *e ) {
	if( !mGlWidget->isRendering() ) {
		QWidget::keyPressEvent( e );
		return;
	}

	switch( e->key() ) {

		case Qt::Key_W:
			mGlWidget->cameraMoveForward();
			break;

		case Qt::Key_S:
			mGlWidget->cameraMoveBackward();
			break;

		case Qt::Key_A:
			mGlWidget->cameraMoveLeft();
			break;

		case Qt::Key_D:
			mGlWidget->cameraMoveRight();
			break;

		case Qt::Key_Q:
			mGlWidget->cameraMoveUp();
			break;

		case Qt::Key_E:
			mGlWidget->cameraMoveDown();
			break;

		case Qt::Key_R:
			mGlWidget->cameraReset();
			break;

		default:
			QWidget::keyPressEvent( e );

	}
}


/**
 * Handle mouse mouve events.
 * @param e {QMouseEvent*} e Mouse event triggered by moving the mouse.
 */
void Window::mouseMoveEvent( QMouseEvent *e ) {
	if( e->buttons() == Qt::LeftButton && mGlWidget->isRendering() ) {
		int diffX = mMouseLastX - e->x();
		int diffY = mMouseLastY - e->y();

		mGlWidget->updateCameraRot( diffX, diffY );

		mMouseLastX = e->x();
		mMouseLastY = e->y();
	}
}


/**
 * Handle mouse press events.
 * @param e {QMouseEvent*} e Mouse event triggered by pressing a button on the mouse.
 */
void Window::mousePressEvent( QMouseEvent *e ) {
	if( e->buttons() == Qt::LeftButton ) {
		mMouseLastX = e->x();
		mMouseLastY = e->y();
	}
}


/**
 * Update the status bar with a message.
 * @param msg {const char*} msg The message to show in the status bar.
 */
void Window::updateStatus( const char *msg ) {
	mStatusBar->showMessage( tr( msg ) );
}
