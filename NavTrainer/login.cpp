#include "login.h"
#include "ui_login.h"
#include "toastnotification.h"
#include <QFileDialog>
#include <QRegularExpression>
#include <QDate>
#include <QPainter>
#include <QPainterPath>
#include <QCalendarWidget>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDialog>

RegisterWidget::RegisterWidget(NavigationDAO *dao, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Login)
    , m_dao(dao)
{
    ui->setupUi(this);
    
    // Inicializar combobox de fecha
    inicializarComboBoxesFecha();
    
    // Conectar señales y slots
    connect(ui->btnSeleccionarAvatar, &QPushButton::clicked, this, &RegisterWidget::onSeleccionarAvatar);
    connect(ui->btnCrearCuenta, &QPushButton::clicked, this, &RegisterWidget::onCrearCuenta);
    connect(ui->btnYaTengoCuenta, &QPushButton::clicked, this, &RegisterWidget::onYaTengoCuenta);
    connect(ui->btnCalendar, &QPushButton::clicked, this, &RegisterWidget::onAbrirCalendario);
}

RegisterWidget::~RegisterWidget()
{
    delete ui;
}

void RegisterWidget::onSeleccionarAvatar()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Seleccionar Avatar"),
        QDir::homePath(),
        tr("Imágenes (*.png *.jpg *.jpeg *.bmp *.gif)")
    );
    
    if (!fileName.isEmpty()) {
        m_avatarImage.load(fileName);
        
        if (!m_avatarImage.isNull()) {
            int targetSize = ui->lblAvatar->width();
            
            // Escalar la imagen para que llene completamente el círculo
            QPixmap originalPixmap = QPixmap::fromImage(m_avatarImage);
            QPixmap scaledPixmap = originalPixmap.scaled(
                targetSize, targetSize,
                Qt::KeepAspectRatioByExpanding,
                Qt::SmoothTransformation
            );
            
            // Centrar y recortar la imagen si es más grande
            QPixmap centeredPixmap = scaledPixmap;
            if (scaledPixmap.width() > targetSize || scaledPixmap.height() > targetSize) {
                int x = (scaledPixmap.width() - targetSize) / 2;
                int y = (scaledPixmap.height() - targetSize) / 2;
                centeredPixmap = scaledPixmap.copy(x, y, targetSize, targetSize);
            }
            
            // Crear una máscara circular
            QPixmap roundedPixmap(targetSize, targetSize);
            roundedPixmap.fill(Qt::transparent);
            
            QPainter painter(&roundedPixmap);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            
            // Dibujar un círculo y usarlo como máscara
            QPainterPath path;
            path.addEllipse(0, 0, targetSize, targetSize);
            painter.setClipPath(path);
            painter.drawPixmap(0, 0, centeredPixmap);
            painter.end();
            
            ui->lblAvatar->setPixmap(roundedPixmap);
            ui->lblAvatar->setScaledContents(false);
            ui->lblAvatar->setText("");  // Quitar el texto "Avatar"
        }
    }
}

void RegisterWidget::onCrearCuenta()
{
    if (!validarFormulario()) {
        return;
    }
    
    QString nickName = ui->txtNickname->text().trimmed();
    QString correo = ui->txtCorreo->text().trimmed();
    QString contrasena = ui->txtContrasena->text();
    
    if (!m_dao) {
        emit mostrarMensaje(tr("No hay conexión a la base de datos."), ToastNotification::Error);
        return;
    }
    
    // Verificar si el nickname ya existe
    if (nickNameExiste(nickName)) {
        emit mostrarMensaje(tr("El nombre de usuario '%1' ya existe. Por favor elija otro.").arg(nickName),
                            ToastNotification::Warning);
        ui->txtNickname->setFocus();
        return;
    }
    
    // Crear usuario con fecha de nacimiento actual por defecto
    QDate birthdate = QDate::currentDate();
    
    try {
        ::User newUser(nickName, correo, contrasena, m_avatarImage, birthdate);
        m_dao->saveUser(newUser);

        emit cuentaCreada(nickName);
        
        // Limpiar formulario
        ui->txtNickname->clear();
        ui->txtCorreo->clear();
        ui->cbxDia->setCurrentIndex(0);
        ui->cbxMes->setCurrentIndex(0);
        ui->spnAno->setValue(2008);
        ui->txtContrasena->clear();
        ui->txtConfirmarContrasena->clear();
        m_avatarImage = QImage();
        ui->lblAvatar->setPixmap(QPixmap());
        ui->lblAvatar->setText("Avatar");
        
        // Cerrar la ventana
        this->close();
        
    } catch (const NavDAOException &e) {
        emit mostrarMensaje(tr("Error al crear la cuenta: %1").arg(e.what()), ToastNotification::Error);
    }
}

void RegisterWidget::onYaTengoCuenta()
{
    emit irAIniciarSesion();
}

bool RegisterWidget::validarFormulario()
{
    QString nickName = ui->txtNickname->text().trimmed();
    QString correo = ui->txtCorreo->text().trimmed();
    int dia = ui->cbxDia->currentText().toInt();
    int mes = ui->cbxMes->currentIndex() + 1;  // currentIndex es 0-based
    int ano = ui->spnAno->value();
    QString contrasena = ui->txtContrasena->text();
    QString confirmarContrasena = ui->txtConfirmarContrasena->text();
    
    // Validar nickname
    if (nickName.isEmpty()) {
        emit mostrarMensaje(tr("Por favor ingrese un nombre de usuario."), ToastNotification::Warning);
        ui->txtNickname->setFocus();
        return false;
    }
    
    // Validar correo vacío
    if (correo.isEmpty()) {
        emit mostrarMensaje(tr("Por favor ingrese un correo electrónico."), ToastNotification::Warning);
        ui->txtCorreo->setFocus();
        return false;
    }
    
    // Validar formato de correo
    if (!validarCorreo(correo)) {
        emit mostrarMensaje(tr("Por favor ingrese un correo electrónico válido."), ToastNotification::Warning);
        ui->txtCorreo->setFocus();
        return false;
    }

    // Validar edad mínima
    QDate fechaNacimiento(ano, mes, dia);
    if (!fechaNacimiento.isValid()) {
        emit mostrarMensaje(tr("Por favor ingrese una fecha de nacimiento válida."), ToastNotification::Warning);
        ui->cbxDia->setFocus();
        return false;
    }
    
    QDate hoy = QDate::currentDate();
    int edad = hoy.year() - fechaNacimiento.year();
    if (hoy.month() < fechaNacimiento.month() || 
        (hoy.month() == fechaNacimiento.month() && hoy.day() < fechaNacimiento.day())) {
        edad--;
    }
    
    if (edad < 16) {
        emit mostrarMensaje(tr("Debes tener al menos 16 años para crear una cuenta."), ToastNotification::Warning);
        ui->spnAno->setFocus();
        return false;
    }
    
    // Validar contraseña vacía
    if (contrasena.isEmpty()) {
        emit mostrarMensaje(tr("Por favor ingrese una contraseña."), ToastNotification::Warning);
        ui->txtContrasena->setFocus();
        return false;
    }
    
    // Validar longitud de contraseña
    if (!validarContrasena(contrasena)) {
        emit mostrarMensaje(tr("La contraseña debe tener al menos 6 caracteres."), ToastNotification::Warning);
        ui->txtContrasena->setFocus();
        return false;
    }
    
    // Validar confirmación de contraseña
    if (contrasena != confirmarContrasena) {
        emit mostrarMensaje(tr("Las contraseñas no coinciden."), ToastNotification::Warning);
        ui->txtConfirmarContrasena->setFocus();
        return false;
    }

    // Si no hay avatar, continuar silenciosamente usando el predeterminado
    if (m_avatarImage.isNull()) {
        emit mostrarMensaje(tr("No seleccionó un avatar; se usará el predeterminado."), ToastNotification::Info);
    }

    return true;
}

bool RegisterWidget::validarCorreo(const QString &correo)
{
    // Expresión regular para validar correo electrónico
    QRegularExpression regex(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
    return regex.match(correo).hasMatch();
}

bool RegisterWidget::validarContrasena(const QString &contrasena)
{
    // Validar que la contraseña tenga al menos 6 caracteres
    return contrasena.length() >= 6;
}

bool RegisterWidget::nickNameExiste(const QString &nickName)
{
    if (!m_dao) return false;
    
    try {
        QMap<QString, ::User> users = m_dao->loadUsers();
        return users.contains(nickName);
    } catch (const NavDAOException &) {
        return false;
    }
}
void RegisterWidget::inicializarComboBoxesFecha()
{
    // Inicializar combo box de días (1-31)
    for (int i = 1; i <= 31; ++i) {
        ui->cbxDia->addItem(QString::number(i));
    }
    ui->cbxDia->setCurrentIndex(0);  // Seleccionar día 1
    
    // Inicializar combo box de meses (nombres en español)
    QStringList meses;
    meses << "Enero" << "Febrero" << "Marzo" << "Abril" << "Mayo" << "Junio"
          << "Julio" << "Agosto" << "Septiembre" << "Octubre" << "Noviembre" << "Diciembre";
    
    foreach (const QString &mes, meses) {
        ui->cbxMes->addItem(mes);
    }
    ui->cbxMes->setCurrentIndex(0);  // Seleccionar enero
}

QDate RegisterWidget::obtenerFechaSeleccionada() const
{
    int dia = ui->cbxDia->currentText().toInt();
    int mes = ui->cbxMes->currentIndex() + 1;  // currentIndex es 0-based
    int ano = ui->spnAno->value();
    
    return QDate(ano, mes, dia);
}

void RegisterWidget::onAbrirCalendario()
{
    // Crear un diálogo con un calendario
    QDialog *dialogoCalendario = new QDialog(this);
    dialogoCalendario->setWindowTitle(tr("Seleccionar Fecha"));
    dialogoCalendario->setModal(true);
    
    QVBoxLayout *layout = new QVBoxLayout(dialogoCalendario);
    
    QCalendarWidget *calendario = new QCalendarWidget(dialogoCalendario);
    
    // Establecer la fecha actual del formulario en el calendario
    QDate fechaActual = obtenerFechaSeleccionada();
    calendario->setSelectedDate(fechaActual);
    
    // Conectar selección de fecha
    connect(calendario, &QCalendarWidget::clicked, this, [this, dialogoCalendario](const QDate &date) {
        onFechaCalendarioSeleccionada(date);
        dialogoCalendario->accept();
    });
    
    layout->addWidget(calendario);
    
    // Añadir botones
    QHBoxLayout *layoutBotones = new QHBoxLayout();
    QPushButton *btnAceptar = new QPushButton(tr("Aceptar"));
    QPushButton *btnCancelar = new QPushButton(tr("Cancelar"));
    
    connect(btnAceptar, &QPushButton::clicked, this, [this, calendario, dialogoCalendario]() {
        onFechaCalendarioSeleccionada(calendario->selectedDate());
        dialogoCalendario->accept();
    });
    
    connect(btnCancelar, &QPushButton::clicked, dialogoCalendario, &QDialog::reject);
    
    layoutBotones->addStretch();
    layoutBotones->addWidget(btnAceptar);
    layoutBotones->addWidget(btnCancelar);
    
    layout->addLayout(layoutBotones);
    
    dialogoCalendario->exec();
    delete dialogoCalendario;
}

void RegisterWidget::onFechaCalendarioSeleccionada(const QDate &date)
{
    // Actualizar los combobox y spinbox con la fecha seleccionada
    ui->cbxDia->setCurrentIndex(date.day() - 1);
    ui->cbxMes->setCurrentIndex(date.month() - 1);
    ui->spnAno->setValue(date.year());
}