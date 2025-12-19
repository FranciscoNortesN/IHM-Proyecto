#ifndef LOGIN_H
#define LOGIN_H

#include <QWidget>
#include <QString>
#include <QImage>
#include "navlib/navigationdao.h"
#include "navlib/navtypes.h"

namespace Ui {
class Login;
}

class RegisterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RegisterWidget(NavigationDAO *dao, QWidget *parent = nullptr);
    ~RegisterWidget();

private slots:
    void onSeleccionarAvatar();
    void onCrearCuenta();
    void onYaTengoCuenta();
    void onAbrirCalendario();
    void onFechaCalendarioSeleccionada(const QDate &date);

private:
    Ui::Login *ui;
    NavigationDAO *m_dao;
    QImage m_avatarImage;
    
    bool validarFormulario();
    bool validarCorreo(const QString &correo);
    bool validarContrasena(const QString &contrasena);
    bool nickNameExiste(const QString &nickName);
    void inicializarComboBoxesFecha();
    QDate obtenerFechaSeleccionada() const;

signals:
    void cuentaCreada(const QString &nickName);
    void irAIniciarSesion();
    void mostrarMensaje(const QString &mensaje, int tipo);
};

#endif // LOGIN_H
