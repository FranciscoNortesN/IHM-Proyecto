#include "problem.h"
#include "ui_problem.h"

ProblemWidget::ProblemWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::problem)
{
    ui->setupUi(this);
}

ProblemWidget::~ProblemWidget()
{
    delete ui;
}
