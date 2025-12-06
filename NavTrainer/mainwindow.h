#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPoint>
#include <QString>

class MapOverlayPanel;

class Carta;

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

private slots:
    void onProblemsButtonClicked();
    void onUserButtonClicked();

private:
    Ui::MainWindow *ui;
    Carta *m_carta = nullptr;
    MapOverlayPanel *m_overlayPanel = nullptr;
    QString m_currentMapTitle;

    void setupMapView();
    void setupOverlayPanel();
    void setupSidebarButtons();
    bool applyOverlayStyle();
    void updateMapTitle(const QString &title);
    void promptForMapChange();
    bool loadMapResource(const QString &resourcePath, const QString &title);
    bool loadMapFromFile(const QString &filePath);
    void handleOverlayDrag(const QPoint &delta);
    void setupShortcuts();
    bool isSidebarCollapsed() const;
    void toggleSidebar(int stackIndex);
};
#endif // MAINWINDOW_H
