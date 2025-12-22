#ifndef MAPOVERLAYPANEL_H
#define MAPOVERLAYPANEL_H

#include <QColor>
#include <QIcon>
#include <QList>
#include <QPoint>
#include <QWidget>
#include <QString>

#include "maptooltypes.h"

class QLabel;
class QToolButton;
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QSpacerItem;
class QButtonGroup;
class QMenu;
class QSlider;
class ToolPaletteButton;

class MapOverlayPanel : public QWidget
{
    Q_OBJECT

public:
    explicit MapOverlayPanel(QWidget *parent = nullptr);

    void setTitle(const QString &title);
    QString title() const;
    void setToolDescriptors(const QList<MapToolDescriptor> &descriptors);
    enum class Mode
    {
        Drag,
        Paint,
        Erase,
        Text,
        Point,
        Line,
        Grid
    };

    void setActiveMode(Mode mode);
    Mode activeMode() const { return m_activeMode; }
    void setCurrentColor(const QColor &color);
    QColor currentColor() const { return m_currentColor; }
    void setPaintSettings(int thickness, int opacityPercent);
    int minimumVisibleHeight() const;

signals:
    void dragDeltaRequested(const QPoint &delta);
    void changeMapRequested();
    void dragModeSelected();
    void paintModeSelected();
    void eraseModeSelected();
    void textModeSelected();
    void pointModeSelected();
    void gridModeSelected();
    void undoRequested();
    void clearEditsRequested();
    void colorPicked(const QColor &color);
    void gridToggled(bool enabled);
    void strokeWidthChanged(int width);
    void strokeOpacityChanged(int percent);
    void toolRequested(const QString &toolId, const QString &resourcePath);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QWidget *m_headerRow = nullptr;
    QLabel *m_titleLabel = nullptr;
    QToolButton *m_changeButton = nullptr;
    QWidget *m_toolsCard = nullptr;
    QVBoxLayout *m_toolsLayout = nullptr;
    QWidget *m_toolPane = nullptr;
    QHBoxLayout *m_toolPaneLayout = nullptr;
    QWidget *m_toolButtonsContainer = nullptr;
    QHBoxLayout *m_toolButtonsLayout = nullptr;
    QWidget *m_actionGridWidget = nullptr;
    QGridLayout *m_actionGridLayout = nullptr;
    QSpacerItem *m_toolPaneSpacer = nullptr;
    QList<MapToolDescriptor> m_toolDescriptors;
    QList<ToolPaletteButton *> m_toolButtons;
    QList<QWidget *> m_dragHandles;
    QButtonGroup *m_modeButtonGroup = nullptr;
    QToolButton *m_dragModeButton = nullptr;
    QToolButton *m_paintModeButton = nullptr;
    QToolButton *m_eraseModeButton = nullptr;
    QToolButton *m_textModeButton = nullptr;
    QToolButton *m_pointModeButton = nullptr;
    QToolButton *m_gridModeButton = nullptr;
    QToolButton *m_undoButton = nullptr;
    QToolButton *m_colorButton = nullptr;
    QToolButton *m_clearButton = nullptr;
    QToolButton *m_settingsButton = nullptr;
    QMenu *m_settingsMenu = nullptr;
    QSlider *m_thicknessSlider = nullptr;
    QSlider *m_opacitySlider = nullptr;
    bool m_updatingSettingsUi = false;
    QColor m_currentColor = QColor(255, 204, 51);
    Mode m_activeMode = Mode::Drag;
    bool m_dragging = false;
    QPoint m_lastGlobalPos;
    QString m_titleText;

    void buildUi();
    void rebuildToolPane();
    void destroyToolButtons();
    void buildActionGrid();
    void createSettingsMenu();
    QToolButton *makeActionButton(const QString &objectName, const QIcon &icon, const QString &toolTip, bool checkable = false);
    void updateColorButtonStyle();
    void handleModeButtonClicked(int id);
    static int modeToId(Mode mode);
    static Mode idToMode(int id);
    void registerDragHandle(QWidget *widget);
    void handleDragEvent(QMouseEvent *event);
    static QPoint mouseGlobalPos(const QMouseEvent *event);
    void updateTitleElision();
    bool isPosInDragHandle(const QPoint &pos) const;
};

#endif // MAPOVERLAYPANEL_H
