#include "user.h"
#include "ui_user.h"
#include <QRegularExpression>
#include "toastnotification.h"

LoginWidget::LoginWidget(NavigationDAO *dao, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::User)
    , m_dao(dao)
{
    ui->setupUi(this);
    
    // Conectar señales y slots
    connect(ui->btnIniciarSesion, &QPushButton::clicked, this, &LoginWidget::onIniciarSesion);
    connect(ui->btnCrearCuenta, &QPushButton::clicked, this, &LoginWidget::onCrearCuenta);
}

LoginWidget::~LoginWidget()
{
    delete ui;
}

void LoginWidget::onIniciarSesion()
{
    QString usuario = ui->txtUsuario->text().trimmed();
    QString contrasena = ui->txtContrasena->text();
    
    // Validar campos vacíos
    if (usuario.isEmpty()) {
        emit mostrarMensaje(tr("Por favor ingrese su nombre de usuario."), ToastNotification::Warning);
        ui->txtUsuario->setFocus();
        return;
    }
    
    if (contrasena.isEmpty()) {
        emit mostrarMensaje(tr("Por favor ingrese su contraseña."), ToastNotification::Warning);
        ui->txtContrasena->setFocus();
        return;
    }
    
    if (!m_dao) {
        emit mostrarMensaje(tr("No hay conexión a la base de datos."), ToastNotification::Error);
        return;
    }
    
    try {
        // Cargar usuarios de la base de datos
        QMap<QString, ::User> usuarios = m_dao->loadUsers();
        
        // Buscar usuario por nickName y contraseña
        bool usuarioEncontrado = false;
        
        if (usuarios.contains(usuario)) {
            ::User usuarioAuth = usuarios.value(usuario);
            if (usuarioAuth.password() == contrasena) {
                usuarioEncontrado = true;
                
                emit sesionIniciada(usuario);
                
                // Limpiar campos
                ui->txtUsuario->clear();
                ui->txtContrasena->clear();
                
                // Cerrar la ventana
                this->close();
            }
        }
        
        if (!usuarioEncontrado) {
            emit mostrarMensaje(tr("Usuario o contraseña incorrectos."), ToastNotification::Warning);
            ui->txtContrasena->clear();
            ui->txtContrasena->setFocus();
        }
        
    } catch (const NavDAOException &e) {
        emit mostrarMensaje(tr("Error al acceder a la base de datos: %1").arg(e.what()), ToastNotification::Error);
    }
}

void LoginWidget::onCrearCuenta()
{
    emit irACrearCuenta();
}
