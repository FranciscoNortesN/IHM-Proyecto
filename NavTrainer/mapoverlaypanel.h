#ifndef MAPOVERLAYPANEL_H
#define MAPOVERLAYPANEL_H

#include <QList>
#include <QPoint>
#include <QWidget>

#include "maptooltypes.h"

class QLabel;
class QToolButton;
class QVBoxLayout;
class QHBoxLayout;
class QSpacerItem;
class ToolPaletteButton;

class MapOverlayPanel : public QWidget
{
    Q_OBJECT

public:
    explicit MapOverlayPanel(QWidget *parent = nullptr);

    void setTitle(const QString &title);
    QString title() const;
    void setToolDescriptors(const QList<MapToolDescriptor> &descriptors);

signals:
    void dragDeltaRequested(const QPoint &delta);
    void changeMapRequested();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QLabel *m_titleLabel = nullptr;
    QToolButton *m_changeButton = nullptr;
    QWidget *m_toolsCard = nullptr;
    QVBoxLayout *m_toolsLayout = nullptr;
    QWidget *m_toolPane = nullptr;
    QHBoxLayout *m_toolPaneLayout = nullptr;
    QSpacerItem *m_toolPaneSpacer = nullptr;
    QList<MapToolDescriptor> m_toolDescriptors;
    QList<ToolPaletteButton *> m_toolButtons;
    QList<QWidget *> m_dragHandles;
    bool m_dragging = false;
    QPoint m_lastGlobalPos;

    void buildUi();
    void rebuildToolPane();
    void destroyToolButtons();
    void registerDragHandle(QWidget *widget);
    void handleDragEvent(QMouseEvent *event);
    static QPoint mouseGlobalPos(const QMouseEvent *event);
};

#endif // MAPOVERLAYPANEL_H
