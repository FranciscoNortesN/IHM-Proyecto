#ifndef CARTA_H
#define CARTA_H

#include <QColor>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHash>
#include <QList>
#include <QPainterPath>
#include <QPoint>
#include <QPointF>

class QString;
class QWheelEvent;
class QMouseEvent;
class QResizeEvent;
class QPixmap;
class QGraphicsPixmapItem;
class QWidget;
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QMimeData;
class QGraphicsItem;
class MapToolItem;
class QGraphicsSimpleTextItem;
class QGraphicsPathItem;

class Carta : public QGraphicsView
{
    Q_OBJECT
    friend class MapToolItem;

public:
    explicit Carta(QWidget *parent = nullptr);

    enum class InteractionMode
    {
        Drag,
        Paint,
        Erase,
        Text
    };

    bool loadMap(const QString &filePath);
    bool setMapPixmap(const QPixmap &pixmap);
    void clearMap();
    void setZoomRange(qreal minFactor, qreal maxFactor);
    void setOverlayWidget(QWidget *widget);
    void moveOverlayBy(const QPoint &delta);
    void resetOverlayPosition();
    void setInteractionMode(InteractionMode mode);
    InteractionMode interactionMode() const { return m_interactionMode; }
    void setDrawingColor(const QColor &color);
    QColor drawingColor() const { return m_drawingColor; }
    void setStrokeWidth(int width);
    void setStrokeOpacity(int opacityPercent);
    void clearUserAnnotations();
    void undoLastAnnotation();

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void scrollContentsBy(int dx, int dy) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    QGraphicsScene m_scene;
    QGraphicsPixmapItem *m_mapItem = nullptr;
    bool m_panning = false;
    QPoint m_lastMousePos;
    qreal m_minZoomRatio = 0.3;
    qreal m_maxZoomRatio = 6.0;
    qreal m_baseScale = 1.0;
    qreal m_currentScale = 1.0;
    bool m_userHasZoomed = false;
    bool m_pendingFitToHeight = false;
    QWidget *m_overlayWidget = nullptr;
    QPointF m_overlayScenePos;
    bool m_overlayUserMoved = false;
    int m_overlayMargin = 18;
    QHash<QString, MapToolItem *> m_activeToolItems;
    bool m_overlayMouseTransparent = false;
    static constexpr int ToolItemDataKey = 1;
    QList<QGraphicsSimpleTextItem *> m_textItems;
    QList<QGraphicsPathItem *> m_strokeItems;
    QList<QGraphicsItem *> m_annotationStack;
    QGraphicsPathItem *m_currentStroke = nullptr;
    QPainterPath m_currentStrokePath;
    InteractionMode m_interactionMode = InteractionMode::Drag;
    QColor m_drawingColor = QColor(255, 204, 51);
    int m_strokeWidth = 4;
    int m_strokeOpacity = 85;
    bool m_painting = false;
    bool m_erasing = false;

    void applyScale(qreal factor);
    void anchorMapToSide();
    void fitMapToViewportHeight();
    qreal minAllowedScale() const;
    qreal maxAllowedScale() const;
    void syncOverlayToScene(bool clampToViewport = true);
    QPoint clampOverlayToViewport(const QPoint &candidate) const;
    void setOverlayDefaultScenePos();
    bool acceptsToolMime(const QMimeData *mime) const;
    bool addToolItem(const QString &toolId, const QString &resourcePath, const QPointF &scenePos);
    bool isToolItem(const QGraphicsItem *item) const;
    void clearToolInstances();
    void setOverlayMouseTransparent(bool enabled);
    void handleToolDragStarted();
    void handleToolDragFinished(MapToolItem *item);
    void handleToolItemDestroyed(MapToolItem *item);
    void removeTool(const QString &toolId);
    bool overlayContainsSceneRect(const QRectF &rect) const;
    bool overlayContainsViewportPoint(const QPoint &point) const;
    void handleTextClick(const QPointF &scenePos);
    void removeTextItems();
    void removeStrokeItems();
    void startStroke(const QPointF &scenePos);
    void extendStroke(const QPointF &scenePos);
    void finishStroke();
    void abortCurrentStroke();
    void eraseAt(const QPointF &scenePos);
    bool removeAnnotationItem(QGraphicsItem *item);
    void registerAnnotation(QGraphicsItem *item);
    void unregisterAnnotation(QGraphicsItem *item);
    bool dispatchWheelEventToTool(QWheelEvent *event);
    QPoint wheelEventViewportPos(const QWheelEvent *event) const;
};

#endif // CARTA_H
