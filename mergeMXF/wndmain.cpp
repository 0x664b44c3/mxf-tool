#include "wndmain.h"
#include "ui_wndmain.h"

wndMain::wndMain(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::wndMain)
{
	ui->setupUi(this);
}

wndMain::~wndMain()
{
	delete ui;
}
