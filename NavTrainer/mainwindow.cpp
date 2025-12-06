#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "carta.h"
#include "mapoverlaypanel.h"

#include <QDebug>
#include <QColor>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QIODevice>
#include <QMessageBox>
#include <QShortcut>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setupMapView();
    setupOverlayPanel();
    setupSidebarButtons();

    // Colapsar la barra lateral al inicio
    if (ui->splitter)
    {
        QList<int> sizes;
        sizes << ui->splitter->width() << 0;  // Mapa ocupa todo el ancho, sidebar colapsada
        ui->splitter->setSizes(sizes);
        ui->splitter->setCollapsible(1, true);  // Hacer que la barra lateral sea colapsable
    }

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
    connect(m_overlayPanel, &MapOverlayPanel::dragModeSelected, this, [this]()
            {
        if (m_carta)
        {
            m_carta->setInteractionMode(Carta::InteractionMode::Drag);
        } });
    connect(m_overlayPanel, &MapOverlayPanel::paintModeSelected, this, [this]()
            {
        if (m_carta)
        {
            m_carta->setInteractionMode(Carta::InteractionMode::Paint);
        } });
    connect(m_overlayPanel, &MapOverlayPanel::eraseModeSelected, this, [this]()
            {
        if (m_carta)
        {
            m_carta->setInteractionMode(Carta::InteractionMode::Erase);
        } });
    connect(m_overlayPanel, &MapOverlayPanel::textModeSelected, this, [this]()
            {
        if (m_carta)
        {
            m_carta->setInteractionMode(Carta::InteractionMode::Text);
        } });
    connect(m_overlayPanel, &MapOverlayPanel::colorPicked, this, [this](const QColor &color)
            {
        if (m_carta)
        {
            m_carta->setDrawingColor(color);
        } });
    connect(m_overlayPanel, &MapOverlayPanel::strokeWidthChanged, this, [this](int value)
            {
        if (m_carta)
        {
            m_carta->setStrokeWidth(value);
        } });
    connect(m_overlayPanel, &MapOverlayPanel::strokeOpacityChanged, this, [this](int value)
            {
        if (m_carta)
        {
            m_carta->setStrokeOpacity(value);
        } });
    connect(m_overlayPanel, &MapOverlayPanel::undoRequested, this, [this]()
            {
        if (m_carta)
        {
            m_carta->undoLastAnnotation();
        } });
    connect(m_overlayPanel, &MapOverlayPanel::clearEditsRequested, this, [this]()
            {
        if (m_carta)
        {
            m_carta->clearUserAnnotations();
        } });

    const QList<MapToolDescriptor> tools = {
        {QStringLiteral("tool_ruler"), tr("Regla"), QStringLiteral(":/assets/ruler.svg")},
        {QStringLiteral("tool_protractor"), tr("Transportador"), QStringLiteral(":/assets/transportador.svg")}};
    m_overlayPanel->setToolDescriptors(tools);

    m_carta->setOverlayWidget(m_overlayPanel);
    m_carta->setDrawingColor(m_overlayPanel->currentColor());

    setupShortcuts();
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

void MainWindow::setupShortcuts()
{
    auto bind = [this](const QKeySequence &sequence, auto &&handler)
    {
        QShortcut *shortcut = new QShortcut(sequence, this);
        connect(shortcut, &QShortcut::activated, this, handler);
    };

    bind(QKeySequence(Qt::Key_D), [this]()
         {
        if (m_overlayPanel)
        {
            m_overlayPanel->setActiveMode(MapOverlayPanel::Mode::Drag);
        } });

    bind(QKeySequence(Qt::Key_P), [this]()
         {
        if (m_overlayPanel)
        {
            m_overlayPanel->setActiveMode(MapOverlayPanel::Mode::Paint);
        } });

    bind(QKeySequence(Qt::Key_E), [this]()
         {
        if (m_overlayPanel)
        {
            m_overlayPanel->setActiveMode(MapOverlayPanel::Mode::Erase);
        } });

    bind(QKeySequence(Qt::Key_T), [this]()
         {
        if (m_overlayPanel)
        {
            m_overlayPanel->setActiveMode(MapOverlayPanel::Mode::Text);
        } });

    bind(QKeySequence::Undo, [this]()
         {
        if (m_carta)
        {
            m_carta->undoLastAnnotation();
        } });

    bind(QKeySequence::Delete, [this]()
         {
        if (m_carta)
        {
            m_carta->clearUserAnnotations();
        } });

    bind(QKeySequence(Qt::CTRL | Qt::Key_O), [this]()
         { promptForMapChange(); });
}

void MainWindow::setupSidebarButtons()
{
    connect(ui->problems_button, &QPushButton::clicked, this, &MainWindow::onProblemsButtonClicked);
    connect(ui->user_button, &QPushButton::clicked, this, &MainWindow::onUserButtonClicked);
}

bool MainWindow::isSidebarCollapsed() const
{
    if (!ui->splitter)
        return true;
    
    QList<int> sizes = ui->splitter->sizes();
    return sizes.size() > 1 && sizes[1] == 0;
}

void MainWindow::toggleSidebar(int stackIndex)
{
    if (!ui->splitter || !ui->stackedWidget_2)
        return;

    bool isCollapsed = isSidebarCollapsed();
    int currentIndex = ui->stackedWidget_2->currentIndex();

    if (isCollapsed)
    {
        // Está colapsada, expandir y cambiar al índice solicitado
        QList<int> sizes;
        sizes << ui->splitter->width() * 0.6 << ui->splitter->width() * 0.4;
        ui->splitter->setSizes(sizes);
        ui->stackedWidget_2->setCurrentIndex(stackIndex);
    }
    else if (currentIndex == stackIndex)
    {
        // Está expandida mostrando el mismo contenido, colapsar
        QList<int> sizes;
        sizes << ui->splitter->width() << 0;
        ui->splitter->setSizes(sizes);
    }
    else
    {
        // Está expandida mostrando otro contenido, solo cambiar el índice
        ui->stackedWidget_2->setCurrentIndex(stackIndex);
    }
}

void MainWindow::onProblemsButtonClicked()
{
    toggleSidebar(1);  // problem_space está en el índice 1
}

void MainWindow::onUserButtonClicked()
{
    toggleSidebar(0);  // user_space está en el índice 0
}
