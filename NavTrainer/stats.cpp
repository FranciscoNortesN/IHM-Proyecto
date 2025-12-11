#include "stats.h"
#include "ui_stats.h"

Stats::Stats(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Stats)
{
    ui->setupUi(this);
}

Stats::~Stats()
{
    delete ui;
}
