#include "InfoWindow.h"

using std::map;
using std::string;


/**
 * Constructor.
 * @param {QWidget*} parent Parent object of this window.
 * @param {CL*}      cl     Class for OpenCL handling.
 */
InfoWindow::InfoWindow( QWidget* parent, CL* cl ) : QWidget( parent, Qt::Window ) {
	mMainLayout = new QFormLayout();
	mMainLayout->setVerticalSpacing( 6 );
	mMainLayout->setMargin( 12 );

	this->setLayout( mMainLayout );
	this->setWindowTitle( tr( "Info" ) );

	mCL = cl;
	this->addKernelNames( mCL->getKernelNames() );

	mTimer = new QTimer( this );
	connect( mTimer, SIGNAL( timeout() ), this, SLOT( updateInfo() ) );
}


/**
 * Add names of the kernels to the layout.
 * @param {std::map<cl_kernel, std::string>} kernelNames Kernel IDs and their names.
 */
void InfoWindow::addKernelNames( map<cl_kernel, string> kernelNames ) {
	QHBoxLayout* row;
	QLabel* label;

	for( map<cl_kernel, string>::iterator it = kernelNames.begin(); it != kernelNames.end(); it++ ) {
		row = new QHBoxLayout();

		label = new QLabel( tr( it->second.c_str() ) );
		label->setMinimumWidth( 120 );
		label->setAlignment( Qt::AlignLeft );
		row->addWidget( label );

		label = new QLabel( tr( "-- ms" ) );
		label->setMinimumWidth( 60 );
		label->setAlignment( Qt::AlignRight );
		row->addWidget( label );
		mKernelLabels[it->first] = label;

		mMainLayout->addRow( row );
	}
}


/**
 * Called when window gets closed.
 * @param {QCloseEvent*} event
 */
void InfoWindow::closeEvent( QCloseEvent* event ) {
	this->stopUpdating();
}


/**
 * Callend when window gets opened.
 * @param {QShowEvent*} event
 */
void InfoWindow::showEvent( QShowEvent* event ) {
	this->startUpdating();
}


/**
 * Start updating the info window contents.
 */
void InfoWindow::startUpdating() {
	float interval = Cfg::get().value<float>( Cfg::INFO_KERNELTIMES );

	if( interval >= 0.1f ) {
		mTimer->start( interval );
	}
}


/**
 * Stop updating the info window contents.
 */
void InfoWindow::stopUpdating() {
	mTimer->stop();
}


/**
 * Update all info window contents.
 */
void InfoWindow::updateInfo() {
	this->updateKernelTimes();
}


/**
 * Update the kernel times.
 */
void InfoWindow::updateKernelTimes() {
	map<cl_kernel, double> kernelTimes = mCL->getKernelTimes();
	char kt[16];

	for( map<cl_kernel, double>::iterator it = kernelTimes.begin(); it != kernelTimes.end(); it++ ) {
		snprintf( kt, 16, "%.2f ms", it->second );
		mKernelLabels[it->first]->setText( tr( kt ) );
	}
}
