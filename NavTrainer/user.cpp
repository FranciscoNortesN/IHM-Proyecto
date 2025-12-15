#include "user.h"
#include "ui_user.h"
#include <QMessageBox>
#include <QRegularExpression>

LoginWidget::LoginWidget(NavigationDAO *dao, QWidget *parent)
    : QWidget(parent)
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
        QMessageBox::warning(this, tr("Error"), tr("Por favor ingrese su nombre de usuario."));
        ui->txtUsuario->setFocus();
        return;
    }
    
    if (contrasena.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Por favor ingrese su contraseña."));
        ui->txtContrasena->setFocus();
        return;
    }
    
    if (!m_dao) {
        QMessageBox::critical(this, tr("Error"), tr("No hay conexión a la base de datos."));
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
                
                QMessageBox::information(
                    this,
                    tr("Éxito"),
                    tr("¡Bienvenido %1!").arg(usuario)
                );
                
                emit sesionIniciada(usuario);
                
                // Limpiar campos
                ui->txtUsuario->clear();
                ui->txtContrasena->clear();
                
                // Cerrar la ventana
                this->close();
            }
        }
        
        if (!usuarioEncontrado) {
            QMessageBox::warning(
                this,
                tr("Error"),
                tr("Usuario o contraseña incorrectos.")
            );
            ui->txtContrasena->clear();
            ui->txtContrasena->setFocus();
        }
        
    } catch (const NavDAOException &e) {
        QMessageBox::critical(
            this,
            tr("Error de Base de Datos"),
            tr("Error al acceder a la base de datos: %1").arg(e.what())
        );
    }
}

void LoginWidget::onCrearCuenta()
{
    emit irACrearCuenta();
}
