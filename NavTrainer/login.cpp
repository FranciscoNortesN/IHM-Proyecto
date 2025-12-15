#include "login.h"
#include "ui_login.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QDate>

RegisterWidget::RegisterWidget(NavigationDAO *dao, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Login)
    , m_dao(dao)
{
    ui->setupUi(this);
    
    // Conectar señales y slots
    connect(ui->btnSeleccionarAvatar, &QPushButton::clicked, this, &RegisterWidget::onSeleccionarAvatar);
    connect(ui->btnCrearCuenta, &QPushButton::clicked, this, &RegisterWidget::onCrearCuenta);
    connect(ui->btnYaTengoCuenta, &QPushButton::clicked, this, &RegisterWidget::onYaTengoCuenta);
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
            // Escalar la imagen para que se ajuste al label circular
            QPixmap scaledPixmap = QPixmap::fromImage(m_avatarImage).scaled(
                ui->lblAvatar->size(),
                Qt::KeepAspectRatioByExpanding,
                Qt::SmoothTransformation
            );
            
            ui->lblAvatar->setPixmap(scaledPixmap);
            ui->lblAvatar->setScaledContents(true);
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
        QMessageBox::critical(this, tr("Error"), tr("No hay conexión a la base de datos."));
        return;
    }
    
    // Verificar si el nickname ya existe
    if (nickNameExiste(nickName)) {
        QMessageBox::warning(
            this,
            tr("Error"),
            tr("El nombre de usuario '%1' ya existe. Por favor elija otro.").arg(nickName)
        );
        ui->txtNickname->setFocus();
        return;
    }
    
    // Crear usuario con fecha de nacimiento actual por defecto
    QDate birthdate = QDate::currentDate();
    
    try {
        ::User newUser(nickName, correo, contrasena, m_avatarImage, birthdate);
        m_dao->saveUser(newUser);
        
        QMessageBox::information(
            this,
            tr("Éxito"),
            tr("Cuenta creada exitosamente para: %1").arg(nickName)
        );
        
        emit cuentaCreada(nickName);
        
        // Limpiar formulario
        ui->txtNickname->clear();
        ui->txtCorreo->clear();
        ui->txtContrasena->clear();
        ui->txtConfirmarContrasena->clear();
        m_avatarImage = QImage();
        ui->lblAvatar->setPixmap(QPixmap());
        ui->lblAvatar->setText("Avatar");
        
    } catch (const NavDAOException &e) {
        QMessageBox::critical(
            this,
            tr("Error de Base de Datos"),
            tr("Error al crear la cuenta: %1").arg(e.what())
        );
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
    QString contrasena = ui->txtContrasena->text();
    QString confirmarContrasena = ui->txtConfirmarContrasena->text();
    
    // Validar nickname
    if (nickName.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Por favor ingrese un nombre de usuario."));
        ui->txtNickname->setFocus();
        return false;
    }
    
    // Validar correo vacío
    if (correo.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Por favor ingrese un correo electrónico."));
        ui->txtCorreo->setFocus();
        return false;
    }
    
    // Validar formato de correo
    if (!validarCorreo(correo)) {
        QMessageBox::warning(this, tr("Error"), tr("Por favor ingrese un correo electrónico válido."));
        ui->txtCorreo->setFocus();
        return false;
    }
    
    // Validar contraseña vacía
    if (contrasena.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Por favor ingrese una contraseña."));
        ui->txtContrasena->setFocus();
        return false;
    }
    
    // Validar longitud de contraseña
    if (!validarContrasena(contrasena)) {
        QMessageBox::warning(
            this,
            tr("Error"),
            tr("La contraseña debe tener al menos 6 caracteres.")
        );
        ui->txtContrasena->setFocus();
        return false;
    }
    
    // Validar confirmación de contraseña
    if (contrasena != confirmarContrasena) {
        QMessageBox::warning(this, tr("Error"), tr("Las contraseñas no coinciden."));
        ui->txtConfirmarContrasena->setFocus();
        return false;
    }
    
    // Validar avatar (opcional pero mostrar advertencia)
    if (m_avatarImage.isNull()) {
        QMessageBox::StandardButton respuesta = QMessageBox::question(
            this,
            tr("Avatar no seleccionado"),
            tr("No ha seleccionado un avatar. ¿Desea continuar sin avatar?"),
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (respuesta == QMessageBox::No) {
            return false;
        }
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
