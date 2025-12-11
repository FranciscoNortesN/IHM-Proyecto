#include "selecpro.h"
#include "ui_selecpro.h"

SelecPro::SelecPro(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SelecPro)
{
    ui->setupUi(this);
}

SelecPro::~SelecPro()
{
    delete ui;
}
