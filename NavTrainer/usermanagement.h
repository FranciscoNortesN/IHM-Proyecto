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
    void onCambiarAvatar();
    void onCambiarContrasena();
    void onDesloguearse();

private:
    Ui::UserManagement *ui;
    NavigationDAO *m_dao;
    QString m_nickName;
    QImage m_avatarImage;
    
    bool validarContrasena(const QString &contrasena);
    void cargarDatosUsuario();

signals:
    void usuarioDesconectado();
    void avatarActualizado(const QString &nickName);
};

#endif // USERMANAGEMENT_H
