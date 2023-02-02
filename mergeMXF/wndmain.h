#ifndef WNDMAIN_H
#define WNDMAIN_H

#include <QMainWindow>

namespace Ui {
class wndMain;
}

class wndMain : public QMainWindow
{
	Q_OBJECT

public:
	explicit wndMain(QWidget *parent = 0);
	~wndMain();

private:
	Ui::wndMain *ui;
};

#endif // WNDMAIN_H
