#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPoint>
#include <QString>

class MapOverlayPanel;
class NavigationDAO;
class LoginWidget;
class RegisterWidget;
class UserManagement;
class ToastNotification;

class Carta;
class SelecPro;
class User;

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    Carta *m_carta = nullptr;
    MapOverlayPanel *m_overlayPanel = nullptr;
    QString m_currentMapTitle;
    bool m_userFirstLaunch = true;
    NavigationDAO *m_dao = nullptr;
    LoginWidget *m_loginWidget = nullptr;
    RegisterWidget *m_registerWidget = nullptr;
    UserManagement *m_userManagement = nullptr;
    ToastNotification *m_toastNotification = nullptr;
    QString m_currentUserNickname;

    void setupMapView();
    void setupOverlayPanel();
    bool applyOverlayStyle();
    void updateMapTitle(const QString &title);
    void promptForMapChange();
    bool loadMapResource(const QString &resourcePath, const QString &title);
    bool loadMapFromFile(const QString &filePath);
    void handleOverlayDrag(const QPoint &delta);
    void setupShortcuts();
    void updateUserAvatar(const QString &nickName);
    void showToast(const QString &message, int type = 0, int durationMs = 5000);

private slots:
    void onProblemButtonClicked();
    void onStatsButtonClicked();
    void onHelpButtonClicked();
    void onUserButtonClicked();
    void onLogoutButtonClicked();
};
#endif // MAINWINDOW_H
