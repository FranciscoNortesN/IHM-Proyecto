#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "carta.h"

#include <QDebug>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_carta = new Carta(this);
    m_carta->setZoomRange(0.4, 8.0);

    auto *mapLayout = new QVBoxLayout(ui->mapa);
    mapLayout->setContentsMargins(0, 0, 0, 0);
    mapLayout->setSpacing(0);
    mapLayout->addWidget(m_carta);
    mapLayout->setStretch(0, 1);

    if (ui->splitter)
    {
        ui->splitter->setStretchFactor(0, 3);
        ui->splitter->setStretchFactor(1, 1);
    }

    if (!m_carta->loadMap(QStringLiteral(":/assets/carta_nautica.jpg")))
    {
        qWarning() << "Failed to load embedded map resource:/assets/carta_nautica.jpg";
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
