#include "usermanagement.h"
#include "ui_usermanagement.h"
#include "imageutils.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QRegularExpression>

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
    
    // Conectar señales y slots
    connect(ui->btnCambiarNombre, &QPushButton::clicked, this, &UserManagement::onCambiarNombre);
    connect(ui->btnGuardarNombre, &QPushButton::clicked, this, &UserManagement::onGuardarNombre);
    connect(ui->btnCambiarCorreo, &QPushButton::clicked, this, &UserManagement::onCambiarCorreo);
    connect(ui->btnGuardarCorreo, &QPushButton::clicked, this, &UserManagement::onGuardarCorreo);
    connect(ui->btnCambiarAvatarPencil, &QPushButton::clicked, this, &UserManagement::onCambiarAvatar);
    connect(ui->btnCambiarContrasena, &QPushButton::clicked, this, &UserManagement::onCambiarContrasena);
    connect(ui->btnDesloguearse, &QPushButton::clicked, this, &UserManagement::onDesloguearse);
    connect(ui->btnCancelar, &QPushButton::clicked, this, &UserManagement::onCancelar);
    connect(ui->txtConfirmarNueva, &QLineEdit::returnPressed, this, &UserManagement::onGuardarContrasena, Qt::UniqueConnection);
    
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
                int targetSize = 200;
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

void UserManagement::ocultarCamposEdicion()
{
    // Nombre
    ui->lblNuevoNombre->setVisible(false);
    ui->txtNuevoNombre->setVisible(false);
    ui->btnGuardarNombre->setVisible(false);
    ui->txtNuevoNombre->clear();

    // Correo
    ui->lblNuevoCorreo->setVisible(false);
    ui->txtNuevoCorreo->setVisible(false);
    ui->btnGuardarCorreo->setVisible(false);
    ui->txtNuevoCorreo->clear();

    // Contraseña
    ui->lblContrasenaActual->setVisible(false);
    ui->txtContrasenaActual->setVisible(false);
    ui->lblContrasenaNueva->setVisible(false);
    ui->txtContrasenaNueva->setVisible(false);
    ui->lblConfirmarNueva->setVisible(false);
    ui->txtConfirmarNueva->setVisible(false);
    ui->txtContrasenaActual->clear();
    ui->txtContrasenaNueva->clear();
    ui->txtConfirmarNueva->clear();

    setBotonesAccionVisibles(true);
    ui->btnCancelar->setVisible(false);
}

void UserManagement::setBotonesAccionVisibles(bool visibles)
{
    ui->btnCambiarNombre->setVisible(visibles);
    ui->btnCambiarCorreo->setVisible(visibles);
    ui->btnCambiarContrasena->setVisible(visibles);
    ui->btnCambiarAvatarPencil->setVisible(visibles);
    ui->btnDesloguearse->setVisible(visibles);
}

void UserManagement::onCambiarNombre()
{
    ocultarCamposEdicion();
    setBotonesAccionVisibles(false);
    ui->btnCancelar->setVisible(true);
    ui->lblNuevoNombre->setVisible(true);
    ui->txtNuevoNombre->setVisible(true);
    ui->btnGuardarNombre->setVisible(true);
    ui->txtNuevoNombre->setText(m_nickName);
    ui->txtNuevoNombre->setFocus();
}

void UserManagement::onGuardarNombre()
{
    if (!m_dao) {
        QMessageBox::critical(this, tr("Error"), tr("No hay conexión a la base de datos."));
        return;
    }

    QString nuevoNombre = ui->txtNuevoNombre->text().trimmed();
    if (nuevoNombre.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Ingrese un nombre de usuario válido."));
        return;
    }

    if (nuevoNombre == m_nickName) {
        ocultarCamposEdicion();
        return;
    }

    try {
        QMap<QString, ::User> usuarios = m_dao->loadUsers();
        if (usuarios.contains(nuevoNombre)) {
            QMessageBox::warning(this, tr("Error"), tr("Ese nombre ya está en uso."));
            return;
        }

        if (!usuarios.contains(m_nickName)) {
            QMessageBox::warning(this, tr("Error"), tr("Usuario no encontrado."));
            return;
        }

        ::User usuario = usuarios.value(m_nickName);
        usuario.setNickName(nuevoNombre);
        m_dao->updateUserNickName(m_nickName, usuario);

        QString anterior = m_nickName;
        m_nickName = nuevoNombre;
        ui->lblNickname->setText(m_nickName);
        emit nickNameActualizado(anterior, m_nickName);
        ocultarCamposEdicion();
        QMessageBox::information(this, tr("Listo"), tr("Nombre actualizado."));
    } catch (const NavDAOException &e) {
        QMessageBox::critical(this, tr("Error de Base de Datos"), tr("No se pudo actualizar: %1").arg(e.what()));
    }
}

void UserManagement::onCambiarCorreo()
{
    ocultarCamposEdicion();
    setBotonesAccionVisibles(false);
    ui->btnCancelar->setVisible(true);
    ui->lblNuevoCorreo->setVisible(true);
    ui->txtNuevoCorreo->setVisible(true);
    ui->btnGuardarCorreo->setVisible(true);
    ui->txtNuevoCorreo->setFocus();
}

void UserManagement::onGuardarCorreo()
{
    if (!m_dao) {
        QMessageBox::critical(this, tr("Error"), tr("No hay conexión a la base de datos."));
        return;
    }

    QString nuevoCorreo = ui->txtNuevoCorreo->text().trimmed();
    if (!validarCorreo(nuevoCorreo)) {
        QMessageBox::warning(this, tr("Error"), tr("Ingrese un correo válido."));
        return;
    }

    try {
        QMap<QString, ::User> usuarios = m_dao->loadUsers();
        if (!usuarios.contains(m_nickName)) {
            QMessageBox::warning(this, tr("Error"), tr("Usuario no encontrado."));
            return;
        }

        ::User usuario = usuarios.value(m_nickName);
        usuario.setEmail(nuevoCorreo);
        m_dao->updateUser(usuario);

        ocultarCamposEdicion();
        QMessageBox::information(this, tr("Listo"), tr("Correo actualizado."));
    } catch (const NavDAOException &e) {
        QMessageBox::critical(this, tr("Error de Base de Datos"), tr("No se pudo actualizar: %1").arg(e.what()));
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
            int targetSize = 200;
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
    ocultarCamposEdicion();
    setBotonesAccionVisibles(false);
    ui->btnCancelar->setVisible(true);

    ui->lblContrasenaActual->setVisible(true);
    ui->txtContrasenaActual->setVisible(true);
    ui->lblContrasenaNueva->setVisible(true);
    ui->txtContrasenaNueva->setVisible(true);
    ui->lblConfirmarNueva->setVisible(true);
    ui->txtConfirmarNueva->setVisible(true);
    ui->txtContrasenaActual->setFocus();
}

void UserManagement::onGuardarContrasena()
{
    if (!ui->lblContrasenaActual->isVisible()) {
        return; // No estamos en modo cambiar contraseña
    }

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
        
        ocultarCamposEdicion();
        QMessageBox::information(this, tr("Listo"), tr("Contraseña actualizada."));
        
    } catch (const NavDAOException &e) {
        QMessageBox::critical(
            this,
            tr("Error de Base de Datos"),
            tr("Error al actualizar contraseña: %1").arg(e.what())
        );
    }
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

void UserManagement::onCancelar()
{
    ocultarCamposEdicion();
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

bool UserManagement::validarCorreo(const QString &correo)
{
    QRegularExpression regex(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
    return regex.match(correo).hasMatch();
}
