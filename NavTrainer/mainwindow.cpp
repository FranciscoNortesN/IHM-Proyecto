#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "carta.h"
#include "mapoverlaypanel.h"

#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QIODevice>
#include <QMessageBox>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setupMapView();
    setupOverlayPanel();

    if (!loadMapResource(QStringLiteral(":/assets/carta_nautica.jpg"), tr("Carta náutica")))
    {
        QMessageBox::warning(this, tr("Error"),
                             tr("No se pudo cargar el mapa embebido 'carta_nautica.jpg'."));
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupMapView()
{
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
}

void MainWindow::setupOverlayPanel()
{
    if (!m_carta)
    {
        return;
    }

    m_overlayPanel = new MapOverlayPanel(m_carta);
    if (!applyOverlayStyle())
    {
        qWarning() << "No se pudo aplicar el estilo del panel flotante";
    }

    connect(m_overlayPanel, &MapOverlayPanel::dragDeltaRequested,
            this, &MainWindow::handleOverlayDrag);
    connect(m_overlayPanel, &MapOverlayPanel::changeMapRequested,
            this, &MainWindow::promptForMapChange);

    const QList<MapToolDescriptor> tools = {
        {QStringLiteral("tool_ruler"), tr("Regla"), QStringLiteral(":/assets/ruler.svg")},
        {QStringLiteral("tool_protractor"), tr("Transportador"), QStringLiteral(":/assets/transportador.svg")}
    };
    m_overlayPanel->setToolDescriptors(tools);

    m_carta->setOverlayWidget(m_overlayPanel);
}

bool MainWindow::applyOverlayStyle()
{
    if (!m_overlayPanel)
    {
        return false;
    }

    QFile qssFile(QStringLiteral(":/assets/styles/map_overlay.qss"));
    if (!qssFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "Failed to load overlay stylesheet" << qssFile.fileName();
        return false;
    }

    m_overlayPanel->setStyleSheet(QString::fromUtf8(qssFile.readAll()));
    return true;
}

void MainWindow::updateMapTitle(const QString &title)
{
    m_currentMapTitle = title;
    if (m_overlayPanel)
    {
        m_overlayPanel->setTitle(title);
    }
}

bool MainWindow::loadMapResource(const QString &resourcePath, const QString &title)
{
    if (!m_carta)
    {
        return false;
    }

    if (!m_carta->loadMap(resourcePath))
    {
        qWarning() << "Failed to load map resource" << resourcePath;
        return false;
    }

    updateMapTitle(title);
    return true;
}

bool MainWindow::loadMapFromFile(const QString &filePath)
{
    if (!m_carta)
    {
        return false;
    }

    if (!m_carta->loadMap(filePath))
    {
        return false;
    }

    const QFileInfo info(filePath);
    updateMapTitle(info.completeBaseName().isEmpty() ? info.fileName() : info.completeBaseName());
    return true;
}

void MainWindow::promptForMapChange()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this, tr("Seleccionar mapa"), QString(),
        tr("Imágenes (*.png *.jpg *.jpeg *.bmp *.tif *.tiff *.svg *.webp);;Todos los archivos (*.*)"));

    if (filePath.isEmpty())
    {
        return;
    }

    if (!loadMapFromFile(filePath))
    {
        QMessageBox::warning(this, tr("Error"),
                             tr("No se pudo cargar el mapa seleccionado."));
    }
}

void MainWindow::handleOverlayDrag(const QPoint &delta)
{
    if (m_carta)
    {
        m_carta->moveOverlayBy(delta);
    }

}
