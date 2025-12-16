#include "usermanagement.h"
#include "ui_usermanagement.h"
#include "imageutils.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>

UserManagement::UserManagement(NavigationDAO *dao, const QString &nickName, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::UserManagement)
    , m_dao(dao)
    , m_nickName(nickName)
{
    ui->setupUi(this);
    // Asegurar tamaño fijo del avatar a 200x200 para evitar recortes
    if (ui->avatarContainer)
        ui->avatarContainer->setFixedSize(200, 200);
    if (ui->lblAvatarPerfil)
        ui->lblAvatarPerfil->setFixedSize(200, 200);
    
    // Conectar señales y slots
    connect(ui->btnCambiarAvatar, &QPushButton::clicked, this, &UserManagement::onCambiarAvatar);
    connect(ui->btnCambiarContrasena, &QPushButton::clicked, this, &UserManagement::onCambiarContrasena);
    connect(ui->btnDesloguearse, &QPushButton::clicked, this, &UserManagement::onDesloguearse);
    
    // Cargar datos del usuario
    cargarDatosUsuario();
}

UserManagement::~UserManagement()
{
    delete ui;
}

void UserManagement::cargarDatosUsuario()
{
    if (!m_dao) {
        return;
    }
    
    try {
        QMap<QString, ::User> usuarios = m_dao->loadUsers();
        
        if (usuarios.contains(m_nickName)) {
            ::User usuario = usuarios.value(m_nickName);
            
            // Mostrar nombre de usuario
            ui->lblNickname->setText(usuario.nickName());
            
            // Cargar y mostrar avatar
            m_avatarImage = usuario.avatar();
            
            if (!m_avatarImage.isNull()) {
                int targetSize = qMin(ui->lblAvatarPerfil->width(), ui->lblAvatarPerfil->height());
                QPixmap roundedPixmap = ImageUtils::makeCircularFromImage(m_avatarImage, targetSize);
                ui->lblAvatarPerfil->setPixmap(roundedPixmap);
                ui->lblAvatarPerfil->setText("");
            }
        }
    } catch (const NavDAOException &e) {
        QMessageBox::critical(
            this,
            tr("Error de Base de Datos"),
            tr("Error al cargar datos del usuario: %1").arg(e.what())
        );
    }
}

void UserManagement::onCambiarAvatar()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Seleccionar Avatar"),
        QDir::homePath(),
        tr("Imágenes (*.png *.jpg *.jpeg *.bmp *.gif)")
    );
    
    if (!fileName.isEmpty()) {
        QImage newAvatar;
        newAvatar.load(fileName);
        
        if (!newAvatar.isNull()) {
            m_avatarImage = newAvatar;
            
            // Mostrar preview
            int targetSize = qMin(ui->lblAvatarPerfil->width(), ui->lblAvatarPerfil->height());
            QPixmap roundedPixmap = ImageUtils::makeCircularFromImage(m_avatarImage, targetSize);
            ui->lblAvatarPerfil->setPixmap(roundedPixmap);
            ui->lblAvatarPerfil->setText("");
            
            // Guardar inmediatamente
            if (!m_dao) {
                QMessageBox::critical(this, tr("Error"), tr("No hay conexión a la base de datos."));
                return;
            }
            
            try {
                QMap<QString, ::User> usuarios = m_dao->loadUsers();
                
                if (usuarios.contains(m_nickName)) {
                    ::User usuario = usuarios.value(m_nickName);
                    usuario.setAvatar(m_avatarImage);
                    m_dao->updateUser(usuario);
                    
                    // Notificar el cambio
                    emit avatarActualizado(m_nickName);
                }
            } catch (const NavDAOException &e) {
                QMessageBox::critical(
                    this,
                    tr("Error de Base de Datos"),
                    tr("Error al actualizar avatar: %1").arg(e.what())
                );
            }
        }
    }
}

void UserManagement::onCambiarContrasena()
{
    // Habilitar los campos de contraseña
    ui->lblContrasenaActual->setVisible(true);
    ui->txtContrasenaActual->setVisible(true);
    ui->lblContrasenaNueva->setVisible(true);
    ui->txtContrasenaNueva->setVisible(true);
    ui->lblConfirmarNueva->setVisible(true);
    ui->txtConfirmarNueva->setVisible(true);
    
    // Conectar cambio automático al presionar Enter en el último campo
    connect(ui->txtConfirmarNueva, &QLineEdit::returnPressed, this, [this]() {
        if (!m_dao) {
            QMessageBox::critical(this, tr("Error"), tr("No hay conexión a la base de datos."));
            return;
        }
        
        QString contrasenaActual = ui->txtContrasenaActual->text();
        QString contrasenaNueva = ui->txtContrasenaNueva->text();
        QString confirmarNueva = ui->txtConfirmarNueva->text();
        
        if (contrasenaActual.isEmpty() || contrasenaNueva.isEmpty() || confirmarNueva.isEmpty()) {
            QMessageBox::warning(this, tr("Error"), tr("Complete todos los campos."));
            return;
        }
        
        try {
            QMap<QString, ::User> usuarios = m_dao->loadUsers();
            
            if (!usuarios.contains(m_nickName)) {
                QMessageBox::warning(this, tr("Error"), tr("Usuario no encontrado."));
                return;
            }
            
            ::User usuario = usuarios.value(m_nickName);
            
            // Validar contraseña actual
            if (usuario.password() != contrasenaActual) {
                QMessageBox::warning(this, tr("Error"), tr("La contraseña actual es incorrecta."));
                return;
            }
            
            // Validar que las nuevas contraseñas coincidan
            if (contrasenaNueva != confirmarNueva) {
                QMessageBox::warning(this, tr("Error"), tr("Las contraseñas no coinciden."));
                return;
            }
            
            // Validar contraseña nueva
            if (!validarContrasena(contrasenaNueva)) {
                return;
            }
            
            // Actualizar y guardar
            usuario.setPassword(contrasenaNueva);
            m_dao->updateUser(usuario);
            
            // Limpiar campos
            ui->txtContrasenaActual->clear();
            ui->txtContrasenaNueva->clear();
            ui->txtConfirmarNueva->clear();
            
            // Ocultar campos nuevamente
            ui->lblContrasenaActual->setVisible(false);
            ui->txtContrasenaActual->setVisible(false);
            ui->lblContrasenaNueva->setVisible(false);
            ui->txtContrasenaNueva->setVisible(false);
            ui->lblConfirmarNueva->setVisible(false);
            ui->txtConfirmarNueva->setVisible(false);
            
        } catch (const NavDAOException &e) {
            QMessageBox::critical(
                this,
                tr("Error de Base de Datos"),
                tr("Error al actualizar contraseña: %1").arg(e.what())
            );
        }
    });
}

void UserManagement::onDesloguearse()
{
    int respuesta = QMessageBox::question(
        this,
        tr("Desloguearse"),
        tr("¿Está seguro de que desea desloguearse?"),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (respuesta == QMessageBox::Yes) {
        emit usuarioDesconectado();
        this->close();
    }
}

bool UserManagement::validarContrasena(const QString &contrasena)
{
    if (contrasena.length() < 6) {
        QMessageBox::warning(
            this,
            tr("Error"),
            tr("La contraseña debe tener al menos 6 caracteres.")
        );
        return false;
    }
    return true;
}
