#include "InfoWindow.h"


InfoWindow::InfoWindow( QWidget* parent ) : QWidget( parent, Qt::Window ) {
	QLabel* label = new QLabel( tr( "lorem ipsum" ) );

	QVBoxLayout* mainLayout = new QVBoxLayout();
	mainLayout->setSpacing( 0 );
	mainLayout->setMargin( 0 );
	mainLayout->addWidget( label );

	this->setLayout( mainLayout );
	this->setWindowTitle( tr( "Information" ) );
}
