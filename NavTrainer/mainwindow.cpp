#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "carta.h"
#include "mapoverlaypanel.h"
#include "problem.h"
#include "selecpro.h"
#include "stats.h"
#include "help.h"
#include "user.h"
#include "login.h"
#include "usermanagement.h"
#include "toastnotification.h"
#include "navlib/navigationdao.h"

#include <QDebug>
#include <QColor>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QIODevice>
#include <QMessageBox>
#include <QShortcut>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("NavTrainer");
    
    // Inicializar la base de datos
    // Usar la base de datos en la carpeta data
    m_dao = new NavigationDAO(QStringLiteral("data/navdb.sqlite"));

    setupMapView();
    setupOverlayPanel();
    
    // Inicializar notificación toast
    m_toastNotification = new ToastNotification(this);
    
    // Inicializar contenedor de problemas minimizados
    m_minimizedProblemsContainer = ui->minimized_problems_container;

    connect(ui->problem_button, &QPushButton::clicked, this, &MainWindow::onProblemButtonClicked);
    connect(ui->stats_button, &QPushButton::clicked, this, &MainWindow::onStatsButtonClicked);
    connect(ui->help_button, &QPushButton::clicked, this, &MainWindow::onHelpButtonClicked);
    connect(ui->user_button, &QPushButton::clicked, this, &MainWindow::onUserButtonClicked);
    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::onLogoutButtonClicked);

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
            m_carta->setCrosshairPlacementEnabled(enabled);
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
    SelecPro *selecProWindow = new SelecPro(m_dao, m_currentUserNickname, this);
    selecProWindow->setAttribute(Qt::WA_DeleteOnClose);
    selecProWindow->setWindowFlags(Qt::Window);
    selecProWindow->setMainWindow(this);
    selecProWindow->show();
}

void MainWindow::onStatsButtonClicked()
{
    Stats *statsWindow = new Stats(m_dao, m_currentUserNickname, this);
    statsWindow->setAttribute(Qt::WA_DeleteOnClose);
    statsWindow->setWindowFlags(Qt::Window);
    statsWindow->setWindowModality(Qt::WindowModal);
    statsWindow->show();
}

void MainWindow::onHelpButtonClicked()
{
    Help *helpWindow = new Help(this);
    helpWindow->setAttribute(Qt::WA_DeleteOnClose);
    helpWindow->setWindowFlags(Qt::Window);
    helpWindow->show();
}

void MainWindow::onLogoutButtonClicked()
{
    // Si no hay usuario logueado, no hacer nada
    if (m_currentUserNickname.isEmpty()) {
        return;
    }
    
    // Confirmar logout
    int respuesta = QMessageBox::question(
        this,
        tr("Cerrar sesión"),
        tr("¿Está seguro de que desea cerrar sesión?"),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (respuesta == QMessageBox::Yes) {
        // Limpiar usuario actual
        m_currentUserNickname.clear();
        
        // Restablecer icono del botón a default
        ui->user_button->setIcon(QIcon(QStringLiteral(":/assets/icons/avatar-default.svg")));
        
        // Cerrar ventana de gestión de usuario si está abierta
        if (m_userManagement) {
            m_userManagement->close();
            m_userManagement = nullptr;
        }
        
        // Mostrar notificación toast
        showToast(tr("Sesión cerrada correctamente"), ToastNotification::Success);
    }
}

void MainWindow::showToast(const QString &message, int type, int durationMs)
{
    if (m_toastNotification) {
        m_toastNotification->showMessage(message, static_cast<ToastNotification::NotificationType>(type), durationMs);
    }
}

void MainWindow::onUserButtonClicked()
{
    // Si ya hay usuario logueado, mostrar gestión de usuario
    if (!m_currentUserNickname.isEmpty()) {
        if (m_userManagement) {
            m_userManagement->exec();
            return;
        }
        
        // Crear la ventana de gestión
        m_userManagement = new UserManagement(m_dao, m_currentUserNickname, nullptr);
        
        // Conectar señal cuando se actualice el avatar
        connect(m_userManagement, &UserManagement::avatarActualizado, this, [this](const QString &nickName) {
            updateUserAvatar(nickName);
        });
        
        connect(m_userManagement, &UserManagement::usuarioDesconectado, this, [this]() {
            m_currentUserNickname.clear();
            // Restablecer icono del botón a default
            ui->user_button->setIcon(QIcon(QStringLiteral(":/assets/icons/avatar-default.svg")));
            m_userManagement = nullptr;
        });

        connect(m_userManagement, &UserManagement::nickNameActualizado, this,
                [this](const QString &anterior, const QString &nuevo) {
            if (m_currentUserNickname == anterior) {
                m_currentUserNickname = nuevo;
            }
            updateUserAvatar(nuevo);
            showToast(tr("Nombre actualizado a %1").arg(nuevo), ToastNotification::Success);
        });
        
        connect(m_userManagement, &QDialog::finished, this, [this]() {
            m_userManagement = nullptr;
        });
        
        m_userManagement->setAttribute(Qt::WA_DeleteOnClose);
        m_userManagement->exec();
        return;
    }
    
    // Si los widgets ya existen, solo mostrar el login
    if (m_loginWidget) {
        m_loginWidget->show();
        m_loginWidget->raise();
        m_loginWidget->activateWindow();
        return;
    }
    
    // Crear los widgets de login y registro
    m_loginWidget = new LoginWidget(m_dao, this);
    m_registerWidget = new RegisterWidget(m_dao, this);
    
    // Conectar señal de inicio de sesión exitoso
    connect(m_loginWidget, &LoginWidget::sesionIniciada, this, [this](const QString &nickName) {
        m_currentUserNickname = nickName;
        updateUserAvatar(nickName);
        showToast(tr("¡Bienvenido %1!").arg(nickName), ToastNotification::Success);
        m_loginWidget->close();
    });

    // Mostrar mensajes no bloqueantes desde los formularios
    connect(m_loginWidget, &LoginWidget::mostrarMensaje, this, [this](const QString &mensaje, int tipo) {
        showToast(mensaje, tipo);
    });
    
    // Conectar señales para cambiar entre pantallas
    connect(m_loginWidget, &LoginWidget::irACrearCuenta, this, [this]() {
        m_loginWidget->hide();
        m_registerWidget->show();
        m_registerWidget->raise();
        m_registerWidget->activateWindow();
    });
    
    connect(m_registerWidget, &RegisterWidget::irAIniciarSesion, this, [this]() {
        m_registerWidget->hide();
        m_loginWidget->show();
        m_loginWidget->raise();
        m_loginWidget->activateWindow();
    });

    connect(m_registerWidget, &RegisterWidget::cuentaCreada, this, [this](const QString &nickName) {
        m_currentUserNickname = nickName;
        updateUserAvatar(nickName);
        showToast(tr("¡Bienvenido %1!").arg(nickName), ToastNotification::Success);
        m_registerWidget->close();
        if (m_loginWidget) {
            m_loginWidget->close();
        }
    });

    connect(m_registerWidget, &RegisterWidget::mostrarMensaje, this, [this](const QString &mensaje, int tipo) {
        showToast(mensaje, tipo);
    });
    
    // Limpiar cuando se cierren las ventanas
    connect(m_loginWidget, &QDialog::finished, this, [this]() {
        m_loginWidget->deleteLater();
        m_loginWidget = nullptr;
    });
    
    connect(m_registerWidget, &QDialog::finished, this, [this]() {
        m_registerWidget->deleteLater();
        m_registerWidget = nullptr;
    });
    
    // Mostrar el login inicialmente (no modal)
    m_loginWidget->show();
}

void MainWindow::updateUserAvatar(const QString &nickName)
{
    if (!m_dao) {
        return;
    }
    
    try {
        QMap<QString, ::User> usuarios = m_dao->loadUsers();
        
        if (usuarios.contains(nickName)) {
            ::User usuario = usuarios.value(nickName);
            QImage avatarImage = usuario.avatar();
            
            if (!avatarImage.isNull()) {
                int targetSize = 68;
                
                // Recortar la imagen a un cuadrado (lado más pequeño)
                QPixmap originalPixmap = QPixmap::fromImage(avatarImage);
                int minSize = qMin(originalPixmap.width(), originalPixmap.height());
                
                // Centrar y recortar a cuadrado
                int x = (originalPixmap.width() - minSize) / 2;
                int y = (originalPixmap.height() - minSize) / 2;
                QPixmap squarePixmap = originalPixmap.copy(x, y, minSize, minSize);
                
                // Escalar al tamaño del botón
                QPixmap scaledPixmap = squarePixmap.scaled(
                    targetSize, targetSize,
                    Qt::KeepAspectRatio,
                    Qt::SmoothTransformation
                );
                
                // Establecer el icono del botón (cuadrado)
                ui->user_button->setIcon(QIcon(scaledPixmap));
                ui->user_button->setIconSize(QSize(68, 68));
            }
        }
    } catch (const NavDAOException &e) {
        qWarning() << "Error al cargar avatar:" << e.what();
    }
}

void MainWindow::addMinimizedProblem(ProblemWidget *problem)
{
    if (m_minimizedProblems.contains(problem)) {
        return;
    }
    
    m_minimizedProblems.append(problem);
    
    // Obtener el nombre del problema desde la ventana
    QString problemTitle = problem->windowTitle();
    
    // Extraer solo el número del problema (ej: "Ejercicio 3" -> "E3")
    QString buttonText = "E";
    QRegularExpression regex("\\d+");
    QRegularExpressionMatch match = regex.match(problemTitle);
    if (match.hasMatch()) {
        buttonText += match.captured();
    }
    
    m_minimizedProblemNames.append(buttonText);
    
    // Crear botón con texto
    QPushButton *button = new QPushButton(buttonText, this);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button->setMinimumHeight(40);
    button->setStyleSheet(
        "QPushButton {"
        "    border: 1px solid rgba(255, 255, 255, 0.25);"
        "    background-color: transparent;"
        "    color: white;"
        "    border-radius: 6px;"
        "    padding: 8px;"
        "    font-weight: bold;"
        "    font-size: 18px;"
        "}"
        "QPushButton:hover {"
        "    border-color: rgba(255, 255, 255, 0.4);"
        "    color: #d0d0d0;"
        "}"
    );
    
    connect(button, &QPushButton::clicked, [this, problem]() {
        problem->setWindowState(Qt::WindowActive);
        problem->show();
        problem->raise();
        problem->activateWindow();
        removeMinimizedProblem(problem);
    });
    
    m_minimizedButtons.append(button);
    
    // Agregar el botón envuelto para ocupar ~80% del ancho
    if (m_minimizedProblemsContainer && m_minimizedProblemsContainer->layout()) {
        QVBoxLayout *vLayout = qobject_cast<QVBoxLayout*>(m_minimizedProblemsContainer->layout());
        if (vLayout) {
            QWidget *row = new QWidget(m_minimizedProblemsContainer);
            QHBoxLayout *hLayout = new QHBoxLayout(row);
            hLayout->setContentsMargins(0, 0, 0, 5);
            hLayout->setSpacing(0);
            hLayout->addStretch(1);
            hLayout->addWidget(button, 8);
            hLayout->addStretch(1);
            vLayout->addWidget(row);
        }
    }
    
    button->show();
}

void MainWindow::removeMinimizedProblem(ProblemWidget *problem)
{
    int index = m_minimizedProblems.indexOf(problem);
    if (index == -1) {
        return;
    }
    
    m_minimizedProblems.removeAt(index);
    m_minimizedProblemNames.removeAt(index);
    QPushButton *button = m_minimizedButtons.takeAt(index);
    
    // Remover del layout (fila contenedora)
    if (m_minimizedProblemsContainer && m_minimizedProblemsContainer->layout()) {
        QWidget *row = button->parentWidget();
        if (row) {
            m_minimizedProblemsContainer->layout()->removeWidget(row);
            row->deleteLater();
        }
    }
}

void MainWindow::updateMinimizedButtonsPosition()
{
    // Ya no es necesario con el nuevo sistema de layout
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
}
