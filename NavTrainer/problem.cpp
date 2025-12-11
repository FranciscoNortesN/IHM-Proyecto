#include "problem.h"
#include "ui_problem.h"

Problem::Problem(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::problem)
{
    ui->setupUi(this);
}

Problem::~Problem()
{
    delete ui;
}
