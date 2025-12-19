#ifndef USERMANAGEMENT_H
#define USERMANAGEMENT_H

#include <QWidget>
#include <QString>
#include <QImage>
#include "navlib/navigationdao.h"
#include "navlib/navtypes.h"

namespace Ui {
class UserManagement;
}

class UserManagement : public QWidget
{
    Q_OBJECT

public:
    explicit UserManagement(NavigationDAO *dao, const QString &nickName, QWidget *parent = nullptr);
    ~UserManagement();

private slots:
    void onCambiarNombre();
    void onGuardarNombre();
    void onCambiarCorreo();
    void onGuardarCorreo();
    void onCambiarAvatar();
    void onCambiarContrasena();
    void onGuardarContrasena();
    void onCancelar();
    void onDesloguearse();

private:
    Ui::UserManagement *ui;
    NavigationDAO *m_dao;
    QString m_nickName;
    QImage m_avatarImage;
    
    bool validarContrasena(const QString &contrasena);
    bool validarCorreo(const QString &correo);
    void ocultarCamposEdicion();
    void setBotonesAccionVisibles(bool visibles);
    void cargarDatosUsuario();

signals:
    void usuarioDesconectado();
    void avatarActualizado(const QString &nickName);
    void nickNameActualizado(const QString &anterior, const QString &nuevo);
};

#endif // USERMANAGEMENT_H
