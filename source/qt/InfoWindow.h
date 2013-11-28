#ifndef INFO_WINDOW_H
#define INFO_WINDOW_H

#include "Window.h"

using std::map;
using std::string;


class InfoWindow : public QWidget {

	Q_OBJECT

	public:
		InfoWindow( QWidget* parent, CL* cl );
		void startUpdating();
		void stopUpdating();

	public slots:
		void updateInfo();

	protected:
		void addKernelNames( map<cl_kernel, string> kernelNames );
		void closeEvent( QCloseEvent* event );
		void showEvent( QShowEvent* event );
		void updateKernelTimes();

	private:
		CL* mCL;
		QFormLayout* mMainLayout;
		QTimer* mTimer;

		map<cl_kernel, QLabel*> mKernelLabels;

};

#endif
