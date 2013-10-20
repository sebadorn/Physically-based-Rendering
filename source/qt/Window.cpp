#include "Window.h"


/**
 * Constructor.
 */
Window::Window() {
	setlocale( LC_ALL, "C" );

	mMouseLastX = 0;
	mMouseLastY = 0;

	mGLWidget = new GLWidget( this );
	mStatusBar = this->createStatusBar();
	mMenuBar = this->createMenuBar();

	this->setLayout( this->createLayout() );
	this->setWindowTitle( tr( Cfg::get().value<std::string>( Cfg::WINDOW_TITLE ).c_str() ) );
}


/**
 * Create the main layout.
 * @return {QBoxLayout*} The main layout.
 */
QBoxLayout* Window::createLayout() {
	QVBoxLayout* mainLayout = new QVBoxLayout();
	mainLayout->setSpacing( 0 );
	mainLayout->setMargin( 0 );
	mainLayout->addWidget( mMenuBar );
	mainLayout->addWidget( mGLWidget );
	mainLayout->addWidget( mStatusBar );

	return mainLayout;
}


/**
 * Create the menu bar.
 * @return {QMenuBar*} The menu bar.
 */
QMenuBar* Window::createMenuBar() {
	// Menu 1: File

	QAction* actionImport = new QAction( tr( "&Import model..." ), this );
	actionImport->setStatusTip( tr( "Import a model." ) );
	connect( actionImport, SIGNAL( triggered() ), this, SLOT( importFile() ) );

	QAction* actionExit = new QAction( tr( "&Exit" ), this );
	actionExit->setShortcuts( QKeySequence::Quit );
	actionExit->setStatusTip( tr( "Quit the application." ) );
	connect( actionExit, SIGNAL( triggered() ), this, SLOT( close() ) );

	QMenu* menuFile = new QMenu( tr( "&File" ) );
	menuFile->addAction( actionImport );
	menuFile->addAction( actionExit );


	// Menu 2: View

	QAction* actionBoundingBox = new QAction( tr( "Toggle &bounding box" ), this );
	actionBoundingBox->setStatusTip( tr( "Bounding box of the model." ) );
	actionBoundingBox->setCheckable( true );
	actionBoundingBox->setChecked( false );
	connect( actionBoundingBox, SIGNAL( triggered() ), mGLWidget, SLOT( toggleViewBoundingBox() ) );

	QAction* actionKdTree = new QAction( tr( "Toggle &kd-tree visualization" ), this );
	actionKdTree->setStatusTip( tr( "Visualize the generated kd-tree." ) );
	actionKdTree->setCheckable( true );
	actionKdTree->setChecked( false );
	connect( actionKdTree, SIGNAL( triggered() ), mGLWidget, SLOT( toggleViewKdTree() ) );

	QAction* actionOverlay = new QAction( tr( "Toggle original &overlay" ), this );
	actionOverlay->setStatusTip( tr( "Translucent overlay of the model over the traced one." ) );
	actionOverlay->setCheckable( true );
	actionOverlay->setChecked( false );
	connect( actionOverlay, SIGNAL( triggered() ), mGLWidget, SLOT( toggleViewOverlay() ) );

	QAction* actionTracer = new QAction( tr( "Toggle path &tracing" ), this );
	actionTracer->setStatusTip( tr( "Path tracing." ) );
	actionTracer->setCheckable( true );
	actionTracer->setChecked( true );
	connect( actionTracer, SIGNAL( triggered() ), mGLWidget, SLOT( toggleViewTracer() ) );

	QMenu* menuView = new QMenu( tr( "&View" ) );
	menuView->addAction( actionBoundingBox );
	menuView->addAction( actionKdTree );
	menuView->addAction( actionOverlay );
	menuView->addAction( actionTracer );


	// The menu bar itself

	QMenuBar* menubar = new QMenuBar( this );
	menubar->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
	menubar->addMenu( menuFile );
	menubar->addMenu( menuView );


	return menubar;
}


/**
 * Import a file through a file dialog.
 */
void Window::importFile() {
	mGLWidget->stopRendering();

	QString fileDialogResult = QFileDialog::getOpenFileName(
		this,
		tr( "Import file" ),
		Cfg::get().value<std::string>( Cfg::IMPORT_PATH ).c_str(),
		tr( "OBJ model (*.obj);;All files (*.*)" )
	);

	std::string filePath = fileDialogResult.toStdString();

	if( filePath.empty() ) {
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

	return statusBar;
}


/**
 * Handle key press events.
 * @param e {QKeyEvent*} e Key event triggered by pressing a key.
 */
void Window::keyPressEvent( QKeyEvent* e ) {
	switch( e->key() ) {

		case Qt::Key_W:
		case Qt::Key_S:
		case Qt::Key_A:
		case Qt::Key_D:
		case Qt::Key_Q:
		case Qt::Key_E:
		case Qt::Key_R:
			mGLWidget->moveCamera( e->key() );
			break;

		case Qt::Key_L:
			mGLWidget->toggleLightControl();
			break;

		case Qt::Key_Plus:
			mGLWidget->selectNextLight();
			break;

		case Qt::Key_Minus:
			mGLWidget->selectPreviousLight();
			break;

		case Qt::Key_F11:
			this->toggleFullscreen();
			break;

		default:
			QWidget::keyPressEvent( e );

	}
}


/**
 * Handle mouse mouve events.
 * @param e {QMouseEvent*} e Mouse event triggered by moving the mouse.
 */
void Window::mouseMoveEvent( QMouseEvent* e ) {
	if( e->buttons() == Qt::LeftButton && mGLWidget->isRendering() ) {
		int diffX = mMouseLastX - e->x();
		int diffY = mMouseLastY - e->y();

		mGLWidget->mCamera->updateCameraRot( diffX, diffY );

		mMouseLastX = e->x();
		mMouseLastY = e->y();
	}
}


/**
 * Handle mouse press events.
 * @param e {QMouseEvent*} e Mouse event triggered by pressing a button on the mouse.
 */
void Window::mousePressEvent( QMouseEvent* e ) {
	if( e->buttons() == Qt::LeftButton ) {
		mMouseLastX = e->x();
		mMouseLastY = e->y();
	}
}


/**
 * Toggle between fullscreen and normal window mode.
 */
void Window::toggleFullscreen() {
	if( this->isFullScreen() ) {
		this->showNormal();
		mMenuBar->show();
	}
	else {
		mMenuBar->hide();
		this->showFullScreen();
	}
}


/**
 * Update the status bar with a message.
 * @param msg {const char*} msg The message to show in the status bar.
 */
void Window::updateStatus( const char* msg ) {
	mStatusBar->showMessage( QString::fromUtf8( msg ) );
}
