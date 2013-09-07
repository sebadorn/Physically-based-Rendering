#include <iostream>
#include <cmath>
#include <QtGui>
#include <QSizePolicy>
#include <string>

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

	mGLWidget = new GLWidget( this );
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
	mainLayout->addWidget( mGLWidget );
	mainLayout->addWidget( mStatusBar );

	return mainLayout;
}


/**
 * Create the menu bar.
 * @return {QMenuBar*} The menu bar.
 */
QMenuBar* Window::createMenuBar() {
	QAction *actionImport = new QAction( tr( "&Import" ), this );
	actionImport->setStatusTip( tr( "Import a model." ) );
	connect( actionImport, SIGNAL( triggered() ), this, SLOT( importFile() ) );

	QAction *actionExit = new QAction( tr( "&Exit" ), this );
	actionExit->setShortcuts( QKeySequence::Quit );
	actionExit->setStatusTip( tr( "Quit the application." ) );
	connect( actionExit, SIGNAL( triggered() ), this, SLOT( close() ) );

	QMenu *menuFile = new QMenu( tr( "&File" ) );
	menuFile->addAction( actionImport );
	menuFile->addAction( actionExit );

	QMenuBar *menubar = new QMenuBar( this );
	menubar->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
	menubar->addMenu( menuFile );

	return menubar;
}


/**
 * Import a file through a file dialog.
 */
void Window::importFile() {
	mGLWidget->stopRendering();

	QString fileDialogResult = QFileDialog::getOpenFileName(
		this, tr( "Import file" ),
		"/home/seba/programming/Physically-based Rendering/resources/",
		tr( "OBJ model (*.obj);;All files (*.*)" )
	);

	std::string filePath = fileDialogResult.toStdString();

	if( filePath.empty() ) {
		std::cout << "* Nothing imported." << std::endl;
		return;
	}

	uint splitHere = filePath.find_last_of( '/' );
	std::string fileName = filePath.substr( splitHere + 1 );
	filePath = filePath.substr( 0, splitHere + 1 );

	mGLWidget->loadModel( filePath, fileName );
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
	if( !mGLWidget->isRendering() ) {
		QWidget::keyPressEvent( e );
		return;
	}

	switch( e->key() ) {

		case Qt::Key_W:
			mGLWidget->cameraMoveForward();
			break;

		case Qt::Key_S:
			mGLWidget->cameraMoveBackward();
			break;

		case Qt::Key_A:
			mGLWidget->cameraMoveLeft();
			break;

		case Qt::Key_D:
			mGLWidget->cameraMoveRight();
			break;

		case Qt::Key_Q:
			mGLWidget->cameraMoveUp();
			break;

		case Qt::Key_E:
			mGLWidget->cameraMoveDown();
			break;

		case Qt::Key_R:
			mGLWidget->cameraReset();
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
	if( e->buttons() == Qt::LeftButton && mGLWidget->isRendering() ) {
		int diffX = mMouseLastX - e->x();
		int diffY = mMouseLastY - e->y();

		mGLWidget->updateCameraRot( diffX, diffY );

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
	mStatusBar->showMessage( QString::fromUtf8( msg ) );
}
