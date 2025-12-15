#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPoint>
#include <QString>

class MapOverlayPanel;
class NavigationDAO;
class LoginWidget;
class RegisterWidget;

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

    void setupMapView();
    void setupOverlayPanel();
    bool applyOverlayStyle();
    void updateMapTitle(const QString &title);
    void promptForMapChange();
    bool loadMapResource(const QString &resourcePath, const QString &title);
    bool loadMapFromFile(const QString &filePath);
    void handleOverlayDrag(const QPoint &delta);
    void setupShortcuts();

private slots:
    void onProblemButtonClicked();
    void onStatsButtonClicked();
    void onHelpButtonClicked();
    void onUserButtonClicked();
};
#endif // MAINWINDOW_H
