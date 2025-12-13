#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "carta.h"
#include "mapoverlaypanel.h"
#include "problem.h"
#include "selecpro.h"
#include "stats.h"
#include "help.h"
#include "user.h"

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

    connect(ui->problem_button, &QPushButton::clicked, this, &MainWindow::onProblemButtonClicked);
    connect(ui->stats_button, &QPushButton::clicked, this, &MainWindow::onStatsButtonClicked);
    connect(ui->help_button, &QPushButton::clicked, this, &MainWindow::onHelpButtonClicked);
    connect(ui->user_button, &QPushButton::clicked, this, &MainWindow::onUserButtonClicked);

    if (!loadMapResource(QStringLiteral(":/assets/carta_nautica.jpg"), tr("Estrecho de Gibraltar")))
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
    connect(m_overlayPanel, &MapOverlayPanel::pointModeSelected, this, [this]()
            {
        if (m_carta)
        {
            m_carta->setInteractionMode(Carta::InteractionMode::Point);
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
    connect(m_overlayPanel, &MapOverlayPanel::gridToggled, this, [this](bool enabled)
            {
        if (m_carta)
        {
            m_carta->setProjectionLinesVisible(enabled);
        } });

    const QList<MapToolDescriptor> tools = {
        {QStringLiteral("tool_ruler"), tr("Regla"), QStringLiteral(":/assets/ruler.svg")},
        {QStringLiteral("tool_protractor"), tr("Transportador"), QStringLiteral(":/assets/transportador.svg")},
        {QStringLiteral("tool_compass"), tr("Compás"), QStringLiteral(":/assets/compass_leg.svg")}};
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

    bind(QKeySequence(Qt::Key_O), [this]()
         {
        if (m_overlayPanel)
        {
            m_overlayPanel->setActiveMode(MapOverlayPanel::Mode::Point);
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

void MainWindow::onProblemButtonClicked()
{
    SelecPro *selecProWindow = new SelecPro(this);
    selecProWindow->setAttribute(Qt::WA_DeleteOnClose);
    selecProWindow->setWindowFlags(Qt::Window);
    selecProWindow->show();
}

void MainWindow::onStatsButtonClicked()
{
    Stats *statsWindow = new Stats(this);
    statsWindow->setAttribute(Qt::WA_DeleteOnClose);
    statsWindow->setWindowFlags(Qt::Window);
    statsWindow->show();
}

void MainWindow::onHelpButtonClicked()
{
    Help *helpWindow = new Help(this);
    helpWindow->setAttribute(Qt::WA_DeleteOnClose);
    helpWindow->setWindowFlags(Qt::Window);
    helpWindow->show();
}

void MainWindow::onUserButtonClicked()
{
    User *userWindow = new User(this);
    userWindow->setAttribute(Qt::WA_DeleteOnClose);
    userWindow->setWindowFlags(Qt::Window);
    
    // Alternar entre modo registro (primera vez) y modo iniciar sesión (segunda vez)
    if (m_userFirstLaunch)
    {
        userWindow->setCurrentIndex(0); // Página de registro
        m_userFirstLaunch = false;
    }
    else
    {
        userWindow->setCurrentIndex(1); // Página de iniciar sesión
        m_userFirstLaunch = true; // Resetear para la próxima vez
    }
    
    userWindow->show();
}
