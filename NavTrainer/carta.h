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
class CompassToolItem;
class RulerToolItem;
class QGraphicsSimpleTextItem;
class QGraphicsPathItem;
class QGraphicsEllipseItem;
class QGraphicsLineItem;
class QContextMenuEvent;
class QKeyEvent;
class QFocusEvent;

class Carta : public QGraphicsView
{
    Q_OBJECT
    friend class MapToolItem;
    friend class CompassToolItem;
    friend class RulerToolItem;

public:
    explicit Carta(QWidget *parent = nullptr);

    enum class InteractionMode
    {
        Drag,
        Paint,
        Erase,
        Text,
        Point,
        Line,
        Grid
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
    void setStrokeWidth(int width);
    void setStrokeOpacity(int opacityPercent);
    void clearUserAnnotations();
    void undoLastAnnotation();
    // Add a tool at the center of the viewport (public wrapper for click-to-add)
    void placeToolAtViewportCenter(const QString &toolId, const QString &resourcePath);
    void setProjectionLinesVisible(bool visible);
    void setCrosshairPlacementEnabled(bool enabled);
    QGraphicsPathItem *addArcAnnotation(const QPointF &center, qreal radius, qreal startAngleDeg, qreal spanAngleDeg, qreal rotationOffsetDeg = 0.0);
    QColor drawingColor() const { return m_drawingColor; }
    int strokeWidth() const { return m_strokeWidth; }
    int strokeOpacity() const { return m_strokeOpacity; }

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
    void contextMenuEvent(QContextMenuEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QGraphicsScene m_scene;
    QGraphicsScene m_toolScene;
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
    QPoint m_overlayViewportPos;
    bool m_overlayUserMoved = false;
    int m_overlayMargin = 18;
    QHash<QString, MapToolItem *> m_activeToolItems;
    bool m_overlayMouseTransparent = false;
    static constexpr int ToolItemDataKey = 1;
    QList<QGraphicsSimpleTextItem *> m_textItems;
    QList<QGraphicsPathItem *> m_strokeItems;
    QList<QGraphicsEllipseItem *> m_pointItems;
    QList<QGraphicsLineItem *> m_lineItems;
    QList<QGraphicsPathItem *> m_arcItems;
    QList<QGraphicsItem *> m_annotationStack;
    QGraphicsPathItem *m_currentStroke = nullptr;
    QPainterPath m_currentStrokePath;
    QGraphicsLineItem *m_linePreview = nullptr;
    QPointF m_lineStartScenePos;
    InteractionMode m_interactionMode = InteractionMode::Drag;
    QColor m_drawingColor = QColor(255, 204, 51);
    int m_strokeWidth = 8;
    int m_strokeOpacity = 85;
    bool m_painting = false;
    bool m_erasing = false;
    bool m_lineDrawing = false;
    bool m_showProjectionLines = false;
    QList<QGraphicsLineItem *> m_projectionLines;
    QGraphicsLineItem *m_currentHLine = nullptr;
    QGraphicsLineItem *m_currentVLine = nullptr;
    bool m_crosshairPlacementMode = false;
    QGraphicsLineItem *m_crosshairHLine = nullptr;
    QGraphicsLineItem *m_crosshairVLine = nullptr;
    QHash<MapToolItem *, QPointF> m_toolViewportPos;
    bool m_toolDragInProgress = false;
    MapToolItem *m_draggedToolItem = nullptr;
    RulerToolItem *m_activeRuler = nullptr;
    CompassToolItem *m_activeCompass = nullptr;
    QPointF m_toolDragOffset;
    bool m_repositioningInForeground = false;

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
    void handlePointClick(const QPointF &scenePos);
    void handleGridClick(const QPointF &scenePos);
    void handleLineClick(const QPointF &scenePos);
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
    void startLineSegment(const QPointF &scenePos);
    void updateLinePreview(const QPointF &scenePos);
    void finishLineSegment(const QPointF &scenePos);
    void cancelLinePreview();
    void removePointItems();
    void removeLineItems();
    void removeArcItems();
    void showAnnotationContextMenu(QGraphicsItem *item, const QPoint &globalPos);
    void changeAnnotationColor(QGraphicsItem *item, const QColor &newColor);
    bool isAnnotationItem(QGraphicsItem *item) const;
    void updateProjectionLines();
    void clearProjectionLines();
    void placeCrosshairAt(const QPointF &scenePos);
    void clearCrosshair();
    MapToolItem *rulerItem() const;
    bool pointHitsRuler(const QPointF &scenePos, MapToolItem *ruler) const;
    QPointF rulerDirection(MapToolItem *ruler) const;
    QPointF applyRulerSnap(const QPointF &prevPoint, const QPointF &candidate) const;
    void storeToolViewportPos(MapToolItem *item);
    void repositionToolsToViewport();
    void drawForeground(QPainter *painter, const QRectF &rect) override;
};

#endif // CARTA_H
