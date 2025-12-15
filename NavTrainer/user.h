#ifndef USER_H
#define USER_H

#include <QWidget>
#include <QString>
#include <QImage>
#include "navlib/navigationdao.h"
#include "navlib/navtypes.h"

namespace Ui {
class User;
}

class LoginWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWidget(NavigationDAO *dao, QWidget *parent = nullptr);
    ~LoginWidget();

private slots:
    void onIniciarSesion();
    void onCrearCuenta();

private:
    Ui::User *ui;
    NavigationDAO *m_dao;

signals:
    void sesionIniciada(const QString &nickName);
    void irACrearCuenta();
};

#endif // USER_H
