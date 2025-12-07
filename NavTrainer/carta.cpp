#include "carta.h"
#include "maptooltypes.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFrame>
#include <QGraphicsEllipseItem>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneWheelEvent>
#include <QGraphicsSimpleTextItem>
#include <QtSvgWidgets/qgraphicssvgitem.h>
#include <QMouseEvent>
#include <QMimeData>
#include <QObject>
#include <QPointer>
#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSizePolicy>
#include <QtSvg/qsvgrenderer.h>
#include <QPen>
#include <QWheelEvent>
#include <QWidget>
#include <QtMath>
#include <QTimer>
#include <QLineEdit>
#include <QLineF>
#include <QInputDialog>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QColorDialog>
#include <QApplication>
#include <algorithm>
#include <cmath>

class MapToolItem : public QGraphicsSvgItem
{
public:
    MapToolItem(const QString &toolId, const QString &resourcePath, Carta *view)
        : QGraphicsSvgItem(resourcePath), m_toolId(toolId), m_view(view)
    {
        setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable |
                 QGraphicsItem::ItemSendsGeometryChanges);
        setAcceptedMouseButtons(Qt::LeftButton);
        setZValue(100.0);
        setData(Carta::ToolItemDataKey, true);
        setAcceptHoverEvents(true);

        m_angleLabel = new QGraphicsSimpleTextItem(this);
        m_angleLabel->setBrush(QBrush(Qt::white));
        QFont font = m_angleLabel->font();
        font.setBold(true);
        font.setPointSizeF(font.pointSizeF() + 2);
        m_angleLabel->setFont(font);
        m_angleLabel->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        m_angleLabel->hide();

        m_angleLabelBg = new QGraphicsRectItem(QRectF(0, 0, 0, 0), this);
        m_angleLabelBg->setBrush(QColor(0, 0, 0, 180));
        m_angleLabelBg->setPen(Qt::NoPen);
        m_angleLabelBg->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        m_angleLabelBg->setZValue(m_angleLabel->zValue() - 0.1);
        m_angleLabelBg->hide();

        m_hideLabelTimer.setInterval(1200);
        m_hideLabelTimer.setSingleShot(true);
        QObject::connect(&m_hideLabelTimer, &QTimer::timeout, [this]()
                         {
            if (m_angleLabel)
            {
                m_angleLabel->hide();
            }
            if (m_angleLabelBg)
            {
                m_angleLabelBg->hide();
            } });
    }

    ~MapToolItem() override
    {
        if (m_view)
        {
            m_view->handleToolItemDestroyed(this);
        }
    }

    QString toolId() const
    {
        return m_toolId;
    }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton && m_view)
        {
            m_view->handleToolDragStarted();
        }
        QGraphicsSvgItem::mousePressEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override
    {
        QGraphicsSvgItem::mouseReleaseEvent(event);
        if (m_view)
        {
            m_view->handleToolDragFinished(this);
        }
    }

    void wheelEvent(QGraphicsSceneWheelEvent *event) override
    {
        if (!event)
        {
            return;
        }

        const QPointF localPos = event->pos();
        const QPointF center = boundingRect().center();
        const qreal radius = boundingRect().width() * 0.25;
        if (QLineF(localPos, center).length() > radius)
        {
            QGraphicsSvgItem::wheelEvent(event);
            return;
        }

        const int deltaValue = event->delta();
        if (deltaValue == 0)
        {
            QGraphicsSvgItem::wheelEvent(event);
            return;
        }

        const qreal degrees = static_cast<qreal>(deltaValue) / 8.0; // Qt delivers eighths of a degree
        m_rotationDeg = std::fmod(m_rotationDeg + degrees, 360.0);
        if (m_rotationDeg < 0)
        {
            m_rotationDeg += 360.0;
        }

        setRotation(m_rotationDeg);
        displayMeasurement(QString::number(qRound(m_rotationDeg)) + QStringLiteral("°"));
        event->accept();
    }

    void displayMeasurement(const QString &text)
    {
        if (!m_angleLabel || !m_angleLabelBg)
        {
            return;
        }

        m_angleLabel->setText(text);
        const QRectF textRect = m_angleLabel->boundingRect();
        const QPointF center = boundingRect().center();
        const QPointF offset(0, -boundingRect().height() * 0.1);
        const QPointF labelPos = center + offset - QPointF(textRect.width() / 2, textRect.height() / 2);
        m_angleLabel->setPos(labelPos);

        QRectF bgRect = textRect.adjusted(-6, -3, 6, 3);
        m_angleLabelBg->setRect(bgRect);
        m_angleLabelBg->setPos(labelPos);

        m_angleLabelBg->show();
        m_angleLabel->show();
        m_hideLabelTimer.start();
    }

    QString m_toolId;
    QPointer<Carta> m_view;
    qreal m_rotationDeg = 0.0;
    QGraphicsSimpleTextItem *m_angleLabel = nullptr;
    QGraphicsRectItem *m_angleLabelBg = nullptr;
    QTimer m_hideLabelTimer;
};

class CompassToolItem : public MapToolItem
{
public:
    CompassToolItem(const QString &toolId, const QString &resourcePath, Carta *view)
        : MapToolItem(toolId, resourcePath, view)
    {
        const QRectF rect = boundingRect();
        m_pivotLocal = QPointF(rect.left(), rect.center().y());
        m_baseLegLength = std::max<qreal>(1.0, rect.width());
        setTransformOriginPoint(m_pivotLocal);
        setRotation(-90.0); // orient the leg vertically by default
    }

    ~CompassToolItem() override
    {
        discardArcPreview();
    }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override
    {
        MapToolItem::mousePressEvent(event);
        if (!event || event->button() != Qt::LeftButton)
        {
            return;
        }

        const QPointF local = event->pos();
        const QRectF rect = boundingRect();
        const qreal threshold = rect.width() * 0.3;
        if (local.x() <= rect.left() + threshold)
        {
            m_dragRegion = DragRegion::Pivot;
            return;
        }

        m_dragRegion = DragRegion::Pencil;
        beginArcDrag(event->scenePos());
        event->accept();
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (m_dragRegion == DragRegion::Pencil && event)
        {
            updateArcDrag(event->scenePos());
            event->accept();
            return;
        }

        MapToolItem::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (m_dragRegion == DragRegion::Pencil && event && event->button() == Qt::LeftButton)
        {
            finalizeArcDrag();
            event->accept();
        }
        m_dragRegion = DragRegion::None;
        MapToolItem::mouseReleaseEvent(event);
    }

private:
    enum class DragRegion
    {
        None,
        Pivot,
        Pencil
    };

    DragRegion m_dragRegion = DragRegion::None;
    QPointF m_pivotLocal;
    QPointF m_arcPivotScenePos;
    qreal m_baseLegLength = 1.0;
    qreal m_startAngleDeg = 0.0;
    qreal m_currentSpanDeg = 0.0;
    qreal m_currentRadius = 0.0;
    QGraphicsPathItem *m_arcPreview = nullptr;

    void beginArcDrag(const QPointF &scenePos)
    {
        m_arcPivotScenePos = mapToScene(m_pivotLocal);
        m_startAngleDeg = angleFromPivot(scenePos);
        m_currentRadius = std::max<qreal>(8.0, QLineF(m_arcPivotScenePos, scenePos).length());
        ensureArcPreview();
        updateArcPreview(0.0);
        displayMeasurement(QStringLiteral("0°"));
    }

    qreal angleFromPivot(const QPointF &scenePos) const
    {
        return QLineF(m_arcPivotScenePos, scenePos).angle();
    }

    void updateArcDrag(const QPointF &scenePos)
    {
        QLineF line(m_arcPivotScenePos, scenePos);
        const qreal radius = std::max<qreal>(8.0, line.length());
        m_currentRadius = radius;
        if (m_baseLegLength > 0.0)
        {
            const qreal newScale = std::clamp(radius / m_baseLegLength, 0.6, 4.0);
            setScale(newScale);
        }

        const qreal angle = angleFromPivot(scenePos);
        const qreal adjustedRotation = 90.0 - angle;
        m_rotationDeg = adjustedRotation;
        setRotation(adjustedRotation);
        m_currentSpanDeg = normalizeSpan(angle - m_startAngleDeg);
        updateArcPreview(m_currentSpanDeg);
        displayMeasurement(QString::number(qRound(m_currentSpanDeg)) + QStringLiteral("°"));
    }

    void finalizeArcDrag()
    {
        if (!m_view)
        {
            discardArcPreview();
            return;
        }

        if (std::abs(m_currentSpanDeg) < 1.0)
        {
            discardArcPreview();
            return;
        }

        m_view->addArcAnnotation(m_arcPivotScenePos, m_currentRadius, m_startAngleDeg, m_currentSpanDeg);
        discardArcPreview();
    }

    void ensureArcPreview()
    {
        if (m_arcPreview)
        {
            return;
        }

        m_arcPreview = new QGraphicsPathItem();
        QPen pen(Qt::white);
        if (m_view)
        {
            pen.setColor(m_view->drawingColor());
            pen.setWidth(std::max(1, m_view->strokeWidth()));
        }
        pen.setStyle(Qt::DashLine);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        QColor color = pen.color();
        color.setAlpha(180);
        pen.setColor(color);
        m_arcPreview->setPen(pen);
        m_arcPreview->setZValue(88.0);
        if (scene())
        {
            scene()->addItem(m_arcPreview);
        }
    }

    void updateArcPreview(qreal spanDeg)
    {
        if (!m_arcPreview)
        {
            return;
        }

        QPainterPath path;
        const QRectF rect(m_arcPivotScenePos.x() - m_currentRadius, m_arcPivotScenePos.y() - m_currentRadius,
                          m_currentRadius * 2, m_currentRadius * 2);
        path.arcMoveTo(rect, m_startAngleDeg);
        path.arcTo(rect, m_startAngleDeg, spanDeg);
        m_arcPreview->setPath(path);
    }

    void discardArcPreview()
    {
        if (!m_arcPreview)
        {
            return;
        }
        if (scene())
        {
            scene()->removeItem(m_arcPreview);
        }
        delete m_arcPreview;
        m_arcPreview = nullptr;
    }

    static qreal normalizeSpan(qreal value)
    {
        while (value > 360.0)
        {
            value -= 360.0;
        }
        while (value < -360.0)
        {
            value += 360.0;
        }
        return value;
    }
};

Carta::Carta(QWidget *parent)
    : QGraphicsView(parent)
{
    setScene(&m_scene);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setRenderHint(QPainter::Antialiasing, false);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setDragMode(QGraphicsView::NoDrag);
    setFrameShape(QFrame::NoFrame);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumWidth(320);
    setAcceptDrops(true);
    if (viewport())
    {
        viewport()->setAcceptDrops(true);
    }
    m_scene.setBackgroundBrush(Qt::black);
}

void Carta::setOverlayWidget(QWidget *widget)
{
    if (m_overlayWidget == widget)
    {
        syncOverlayToScene(true);
        return;
    }

    setOverlayMouseTransparent(false);

    if (m_overlayWidget && m_overlayWidget->parent() == viewport())
    {
        m_overlayWidget->hide();
        m_overlayWidget->setParent(nullptr);
    }

    m_overlayWidget = widget;

    if (!m_overlayWidget)
    {
        return;
    }

    m_overlayWidget->setParent(viewport());
    m_overlayWidget->raise();
    m_overlayWidget->show();

    connect(m_overlayWidget, &QObject::destroyed, this, [this]()
            {
        setOverlayMouseTransparent(false);
        m_overlayWidget = nullptr; });

    resetOverlayPosition();
}

void Carta::moveOverlayBy(const QPoint &delta)
{
    if (!m_overlayWidget || delta.isNull())
    {
        return;
    }

    const QPoint desired = clampOverlayToViewport(m_overlayWidget->pos() + delta);
    m_overlayWidget->move(desired);
    m_overlayScenePos = mapToScene(desired);
    m_overlayUserMoved = true;
}

void Carta::resetOverlayPosition()
{
    if (!m_overlayWidget)
    {
        return;
    }

    m_overlayUserMoved = false;
    setOverlayDefaultScenePos();
    syncOverlayToScene(true);
}

void Carta::setInteractionMode(InteractionMode mode)
{
    if (m_interactionMode == mode)
    {
        return;
    }

    if (m_interactionMode == InteractionMode::Paint && m_currentStroke)
    {
        finishStroke();
    }

    if (m_interactionMode == InteractionMode::Line && mode != InteractionMode::Line)
    {
        cancelLinePreview();
    }

    if (mode != InteractionMode::Erase)
    {
        m_erasing = false;
    }

    if (mode != InteractionMode::Drag && m_panning)
    {
        m_panning = false;
        unsetCursor();
    }

    m_interactionMode = mode;
}

void Carta::setDrawingColor(const QColor &color)
{
    if (!color.isValid())
    {
        return;
    }
    m_drawingColor = color;

    if (m_currentStroke)
    {
        QPen pen = m_currentStroke->pen();
        QColor c = color;
        c.setAlphaF(std::clamp(m_strokeOpacity / 100.0, 0.0, 1.0));
        pen.setColor(c);
        m_currentStroke->setPen(pen);
    }
}

void Carta::setStrokeWidth(int width)
{
    if (width <= 0)
    {
        return;
    }
    m_strokeWidth = width;
    if (m_currentStroke)
    {
        QPen pen = m_currentStroke->pen();
        pen.setWidth(m_strokeWidth);
        m_currentStroke->setPen(pen);
    }
}

void Carta::setStrokeOpacity(int opacityPercent)
{
    m_strokeOpacity = std::clamp(opacityPercent, 1, 100);
    if (m_currentStroke)
    {
        QPen pen = m_currentStroke->pen();
        QColor c = pen.color();
        c.setAlphaF(std::clamp(m_strokeOpacity / 100.0, 0.0, 1.0));
        pen.setColor(c);
        m_currentStroke->setPen(pen);
    }
}

void Carta::clearUserAnnotations()
{
    abortCurrentStroke();
    cancelLinePreview();
    clearToolInstances();
    removeTextItems();
    removeStrokeItems();
    removePointItems();
    removeLineItems();
    removeArcItems();
    m_annotationStack.clear();
    m_erasing = false;
}

void Carta::undoLastAnnotation()
{
    while (!m_annotationStack.isEmpty())
    {
        QGraphicsItem *item = m_annotationStack.takeLast();
        if (!item)
        {
            continue;
        }

        if (removeAnnotationItem(item))
        {
            break;
        }
    }
}

bool Carta::loadMap(const QString &filePath)
{
    QPixmap pixmap(filePath);
    return setMapPixmap(pixmap);
}

bool Carta::setMapPixmap(const QPixmap &pixmap)
{
    if (pixmap.isNull())
    {
        return false;
    }

    clearMap();

    m_mapItem = m_scene.addPixmap(pixmap);
    m_mapItem->setTransformationMode(Qt::SmoothTransformation);
    m_scene.setSceneRect(pixmap.rect());
    m_userHasZoomed = false;
    m_pendingFitToHeight = true;
    fitMapToViewportHeight();
    syncOverlayToScene();
    return true;
}

void Carta::clearMap()
{
    abortCurrentStroke();
    cancelLinePreview();
    clearToolInstances();
    removeTextItems();
    removeStrokeItems();
    removePointItems();
    removeLineItems();
    removeArcItems();
    m_annotationStack.clear();
    m_erasing = false;

    if (m_mapItem)
    {
        m_scene.removeItem(m_mapItem);
        delete m_mapItem;
        m_mapItem = nullptr;
    }

    m_scene.setSceneRect({});
    resetTransform();
    m_baseScale = 1.0;
    m_currentScale = 1.0;
    setOverlayDefaultScenePos();
    syncOverlayToScene(true);
}

void Carta::setZoomRange(qreal minFactor, qreal maxFactor)
{
    if (minFactor <= 0 || maxFactor <= 0 || minFactor >= maxFactor)
    {
        return;
    }

    m_minZoomRatio = minFactor;
    m_maxZoomRatio = maxFactor;
}

void Carta::wheelEvent(QWheelEvent *event)
{
    if (dispatchWheelEventToTool(event))
    {
        return;
    }

    if (!m_mapItem)
    {
        event->ignore();
        return;
    }

    QPointF delta = event->angleDelta();
    if (delta.isNull())
    {
        delta = event->pixelDelta();
    }

    if (delta.y() == 0)
    {
        event->ignore();
        return;
    }

    constexpr qreal zoomStep = 1.15;
    const qreal factor = delta.y() > 0 ? zoomStep : (1.0 / zoomStep);
    applyScale(factor);
    event->accept();
    syncOverlayToScene();
}

void Carta::mousePressEvent(QMouseEvent *event)
{
    setFocus();
    
    if (event->button() == Qt::LeftButton && m_mapItem)
    {
        QGraphicsItem *clickedItem = itemAt(event->pos());
        if (!isToolItem(clickedItem))
        {
            const QPointF scenePos = mapToScene(event->pos());
            switch (m_interactionMode)
            {
            case InteractionMode::Text:
                handleTextClick(scenePos);
                event->accept();
                return;
            case InteractionMode::Paint:
                startStroke(scenePos);
                event->accept();
                return;
            case InteractionMode::Erase:
                m_erasing = true;
                eraseAt(scenePos);
                event->accept();
                return;
            case InteractionMode::Point:
                handlePointClick(scenePos);
                event->accept();
                return;
            case InteractionMode::Line:
                handleLineClick(scenePos);
                event->accept();
                return;
            case InteractionMode::Drag:
                m_panning = true;
                m_lastMousePos = event->pos();
                setCursor(Qt::ClosedHandCursor);
                event->accept();
                return;
            }
        }
    }

    QGraphicsView::mousePressEvent(event);
}

void Carta::mouseMoveEvent(QMouseEvent *event)
{
    if (m_currentStroke)
    {
        extendStroke(mapToScene(event->pos()));
        event->accept();
        return;
    }

    if (m_erasing && (event->buttons() & Qt::LeftButton))
    {
        eraseAt(mapToScene(event->pos()));
        event->accept();
        return;
    }

    if (m_lineDrawing && m_interactionMode == InteractionMode::Line)
    {
        updateLinePreview(mapToScene(event->pos()));
        event->accept();
        return;
    }

    if (m_panning)
    {
        const QPoint delta = event->pos() - m_lastMousePos;
        m_lastMousePos = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void Carta::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (m_currentStroke)
        {
            finishStroke();
            event->accept();
            return;
        }

        if (m_erasing)
        {
            m_erasing = false;
            event->accept();
            return;
        }

        if (m_panning)
        {
            m_panning = false;
            unsetCursor();
            event->accept();
            return;
        }
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void Carta::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    if (!m_mapItem)
    {
        return;
    }

    if (m_pendingFitToHeight || !m_userHasZoomed)
    {
        fitMapToViewportHeight();
    }
    else
    {
        anchorMapToSide();
    }

    syncOverlayToScene();
}

void Carta::scrollContentsBy(int dx, int dy)
{
    QGraphicsView::scrollContentsBy(dx, dy);
    syncOverlayToScene();
}

void Carta::dragEnterEvent(QDragEnterEvent *event)
{
    if (acceptsToolMime(event->mimeData()))
    {
        event->acceptProposedAction();
        return;
    }

    QGraphicsView::dragEnterEvent(event);
}

void Carta::dragMoveEvent(QDragMoveEvent *event)
{
    if (acceptsToolMime(event->mimeData()))
    {
        event->acceptProposedAction();
        return;
    }

    QGraphicsView::dragMoveEvent(event);
}

void Carta::dropEvent(QDropEvent *event)
{
    if (!acceptsToolMime(event->mimeData()))
    {
        QGraphicsView::dropEvent(event);
        return;
    }

    const QString toolId = QString::fromUtf8(event->mimeData()->data(MapToolMime::ToolId));
    const QString resourcePath = QString::fromUtf8(event->mimeData()->data(MapToolMime::ToolSource));
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QPoint widgetPos = event->position().toPoint();
#else
    const QPoint widgetPos = event->pos();
#endif
    const QPointF scenePos = mapToScene(widgetPos);

    if (addToolItem(toolId, resourcePath, scenePos))
    {
        event->acceptProposedAction();
    }
    else
    {
        QGraphicsView::dropEvent(event);
    }
}

void Carta::applyScale(qreal factor)
{
    if (!m_mapItem || qFuzzyIsNull(factor))
    {
        return;
    }

    qreal targetScale = m_currentScale * factor;
    const qreal minScale = minAllowedScale();
    const qreal maxScale = maxAllowedScale();

    if (targetScale > maxScale)
    {
        if (m_currentScale >= maxScale)
        {
            return;
        }
        targetScale = maxScale;
        factor = targetScale / m_currentScale;
    }
    else if (targetScale < minScale)
    {
        if (m_currentScale <= minScale)
        {
            return;
        }
        targetScale = minScale;
        factor = targetScale / m_currentScale;
    }

    if (qFuzzyCompare(factor, 1.0))
    {
        return;
    }

    scale(factor, factor);
    m_currentScale = targetScale;
    m_userHasZoomed = true;
    m_pendingFitToHeight = false;
}

void Carta::anchorMapToSide()
{
    if (!m_mapItem)
    {
        return;
    }

    const bool mapNarrowerThanView = m_mapItem->boundingRect().width() * m_currentScale <= viewport()->width();
    if (mapNarrowerThanView)
    {
        centerOn(m_mapItem);
        horizontalScrollBar()->setValue(horizontalScrollBar()->minimum());
    }

    const bool mapShorterThanView = m_mapItem->boundingRect().height() * m_currentScale <= viewport()->height();
    if (mapShorterThanView)
    {
        verticalScrollBar()->setValue(verticalScrollBar()->minimum());
    }
}

void Carta::fitMapToViewportHeight()
{
    if (!m_mapItem)
    {
        return;
    }

    const int viewHeight = viewport()->height();
    const qreal pixHeight = m_mapItem->boundingRect().height();
    if (viewHeight <= 0 || pixHeight <= 0)
    {
        m_pendingFitToHeight = true;
        return;
    }

    const qreal factor = static_cast<qreal>(viewHeight) / pixHeight;
    if (qFuzzyIsNull(factor))
    {
        m_pendingFitToHeight = true;
        return;
    }

    resetTransform();
    scale(factor, factor);
    m_baseScale = factor;
    m_currentScale = factor;
    m_pendingFitToHeight = false;
    horizontalScrollBar()->setValue(horizontalScrollBar()->minimum());
    verticalScrollBar()->setValue(verticalScrollBar()->minimum());
    anchorMapToSide();
    syncOverlayToScene();
}

qreal Carta::minAllowedScale() const
{
    return m_baseScale * m_minZoomRatio;
}

qreal Carta::maxAllowedScale() const
{
    return m_baseScale * m_maxZoomRatio;
}

void Carta::syncOverlayToScene(bool clampToViewport)
{
    if (!m_overlayWidget)
    {
        return;
    }

    if (scene())
    {
        const QRectF rect = scene()->sceneRect();
        m_overlayScenePos.setX(std::clamp(m_overlayScenePos.x(), rect.left(), rect.right()));
        m_overlayScenePos.setY(std::clamp(m_overlayScenePos.y(), rect.top(), rect.bottom()));
    }

    QPointF viewportPointF = mapFromScene(m_overlayScenePos);
    QPoint desired = viewportPointF.toPoint();
    if (clampToViewport)
    {
        desired = clampOverlayToViewport(desired);
    }

    m_overlayWidget->move(desired);
}

QPoint Carta::clampOverlayToViewport(const QPoint &candidate) const
{
    if (!m_overlayWidget || !viewport())
    {
        return candidate;
    }

    const int maxX = std::max(0, viewport()->width() - m_overlayWidget->width());
    const int maxY = std::max(0, viewport()->height() - m_overlayWidget->height());
    QPoint clamped(candidate);
    clamped.setX(std::clamp(clamped.x(), 0, maxX));
    clamped.setY(std::clamp(clamped.y(), 0, maxY));
    return clamped;
}

void Carta::setOverlayDefaultScenePos()
{
    const QPoint defaultViewportPoint(m_overlayMargin, m_overlayMargin);
    m_overlayScenePos = mapToScene(defaultViewportPoint);
}

bool Carta::acceptsToolMime(const QMimeData *mime) const
{
    return mime && mime->hasFormat(MapToolMime::ToolSource) && mime->hasFormat(MapToolMime::ToolId);
}

bool Carta::addToolItem(const QString &toolId, const QString &resourcePath, const QPointF &scenePos)
{
    if (toolId.isEmpty() || resourcePath.isEmpty())
    {
        return false;
    }

    if (MapToolItem *existing = m_activeToolItems.value(toolId, nullptr))
    {
        const QRectF bounds = existing->boundingRect();
        existing->setPos(scenePos - bounds.center());
        existing->update();
        return true;
    }

    MapToolItem *item = nullptr;
    if (toolId == QLatin1String("tool_compass"))
    {
        item = new CompassToolItem(toolId, resourcePath, this);
    }
    else
    {
        item = new MapToolItem(toolId, resourcePath, this);
    }
    if (!item->renderer() || !item->renderer()->isValid())
    {
        delete item;
        return false;
    }

    const QRectF bounds = item->boundingRect();
    if (toolId != QLatin1String("tool_compass"))
    {
        item->setTransformOriginPoint(bounds.center());
    }
    item->setPos(scenePos - bounds.center());

    m_scene.addItem(item);
    m_activeToolItems.insert(toolId, item);
    return true;
}

bool Carta::isToolItem(const QGraphicsItem *item) const
{
    return item && item->data(ToolItemDataKey).toBool();
}

void Carta::clearToolInstances()
{
    for (auto it = m_activeToolItems.begin(); it != m_activeToolItems.end(); ++it)
    {
        if (MapToolItem *item = it.value())
        {
            m_scene.removeItem(item);
            delete item;
        }
    }

    m_activeToolItems.clear();
}

void Carta::setOverlayMouseTransparent(bool enabled)
{
    if (!m_overlayWidget)
    {
        m_overlayMouseTransparent = false;
        return;
    }

    if (m_overlayMouseTransparent == enabled)
    {
        return;
    }

    m_overlayWidget->setAttribute(Qt::WA_TransparentForMouseEvents, enabled);
    m_overlayMouseTransparent = enabled;
}

void Carta::handleToolDragStarted()
{
    setOverlayMouseTransparent(true);
}

void Carta::handleToolDragFinished(MapToolItem *item)
{
    setOverlayMouseTransparent(false);

    if (!item)
    {
        return;
    }

    if (overlayContainsSceneRect(item->sceneBoundingRect()))
    {
        removeTool(item->toolId());
    }
}

void Carta::handleToolItemDestroyed(MapToolItem *item)
{
    if (!item)
    {
        return;
    }

    const QString key = m_activeToolItems.key(item);
    if (!key.isEmpty())
    {
        m_activeToolItems.remove(key);
    }
}

void Carta::removeTool(const QString &toolId)
{
    if (toolId.isEmpty())
    {
        return;
    }

    MapToolItem *item = m_activeToolItems.take(toolId);
    if (!item)
    {
        return;
    }

    m_scene.removeItem(item);
    delete item;
}

bool Carta::overlayContainsSceneRect(const QRectF &rect) const
{
    if (!rect.isValid())
    {
        return false;
    }

    const QPointF viewportPointF = mapFromScene(rect.center());
    const QPoint viewportPoint = QPointF(viewportPointF).toPoint();
    return overlayContainsViewportPoint(viewportPoint);
}

bool Carta::overlayContainsViewportPoint(const QPoint &point) const
{
    if (!m_overlayWidget)
    {
        return false;
    }

    return m_overlayWidget->geometry().contains(point);
}

bool Carta::dispatchWheelEventToTool(QWheelEvent *event)
{
    if (!event)
    {
        return false;
    }

    const QPoint viewPos = wheelEventViewportPos(event);
    if (!viewport()->rect().contains(viewPos))
    {
        return false;
    }

    QGraphicsItem *item = itemAt(viewPos);
    if (!isToolItem(item))
    {
        return false;
    }

    QGraphicsView::wheelEvent(event);
    return event->isAccepted();
}

QPoint Carta::wheelEventViewportPos(const QWheelEvent *event) const
{
    if (!event)
    {
        return QPoint();
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->position().toPoint();
#else
    return event->pos();
#endif
}

void Carta::handleTextClick(const QPointF &scenePos)
{
    bool ok = false;
    const QString input = QInputDialog::getText(this, tr("Añadir texto"), tr("Contenido"),
                                                QLineEdit::Normal, QString(), &ok);
    const QString text = input.trimmed();
    if (!ok || text.isEmpty())
    {
        return;
    }

    auto *textItem = new QGraphicsSimpleTextItem(text);
    QFont font = textItem->font();
    font.setPointSize(36);
    font.setWeight(QFont::DemiBold);
    textItem->setFont(font);
    textItem->setBrush(QBrush(m_drawingColor));
    const QRectF bounds = textItem->boundingRect();
    textItem->setPos(scenePos - bounds.center());
    textItem->setZValue(95.0);
    m_scene.addItem(textItem);
    m_textItems.append(textItem);
    registerAnnotation(textItem);
}

void Carta::handlePointClick(const QPointF &scenePos)
{
    const qreal radius = std::max<qreal>(4.0, m_strokeWidth * 1.2);
    auto *pointItem = new QGraphicsEllipseItem(-radius, -radius, radius * 2, radius * 2);
    QColor fill = m_drawingColor;
    fill.setAlphaF(std::clamp(m_strokeOpacity / 100.0, 0.0, 1.0));
    pointItem->setBrush(QBrush(fill));
    QPen pen(Qt::white);
    pen.setWidth(std::max(1, m_strokeWidth / 2));
    pen.setColor(fill.darker(150));
    pointItem->setPen(pen);
    pointItem->setPos(scenePos);
    pointItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
    pointItem->setZValue(93.0);
    m_scene.addItem(pointItem);
    m_pointItems.append(pointItem);
    registerAnnotation(pointItem);
}

void Carta::handleLineClick(const QPointF &scenePos)
{
    if (!m_lineDrawing)
    {
        startLineSegment(scenePos);
        return;
    }

    finishLineSegment(scenePos);
}

void Carta::removeTextItems()
{
    for (QGraphicsSimpleTextItem *item : m_textItems)
    {
        if (!item)
        {
            continue;
        }
        unregisterAnnotation(item);
        m_scene.removeItem(item);
        delete item;
    }
    m_textItems.clear();
}

void Carta::removeStrokeItems()
{
    for (QGraphicsPathItem *item : m_strokeItems)
    {
        if (!item)
        {
            continue;
        }
        unregisterAnnotation(item);
        m_scene.removeItem(item);
        delete item;
    }
    m_strokeItems.clear();
}

void Carta::removePointItems()
{
    for (QGraphicsEllipseItem *item : m_pointItems)
    {
        if (!item)
        {
            continue;
        }
        unregisterAnnotation(item);
        m_scene.removeItem(item);
        delete item;
    }
    m_pointItems.clear();
}

void Carta::removeLineItems()
{
    for (QGraphicsLineItem *item : m_lineItems)
    {
        if (!item)
        {
            continue;
        }
        unregisterAnnotation(item);
        m_scene.removeItem(item);
        delete item;
    }
    m_lineItems.clear();
}

void Carta::removeArcItems()
{
    for (QGraphicsPathItem *item : m_arcItems)
    {
        if (!item)
        {
            continue;
        }
        unregisterAnnotation(item);
        m_scene.removeItem(item);
        delete item;
    }
    m_arcItems.clear();
}

void Carta::startLineSegment(const QPointF &scenePos)
{
    cancelLinePreview();
    m_lineStartScenePos = scenePos;
    auto *preview = new QGraphicsLineItem(QLineF(scenePos, scenePos));
    QPen pen(m_drawingColor);
    QColor color = m_drawingColor;
    color.setAlphaF(0.7f);
    pen.setColor(color);
    pen.setWidth(std::max(1, m_strokeWidth));
    pen.setStyle(Qt::DashLine);
    pen.setCapStyle(Qt::RoundCap);
    preview->setPen(pen);
    preview->setZValue(89.0);
    m_scene.addItem(preview);
    m_linePreview = preview;
    m_lineDrawing = true;
}

void Carta::updateLinePreview(const QPointF &scenePos)
{
    if (!m_linePreview)
    {
        return;
    }

    m_linePreview->setLine(QLineF(m_lineStartScenePos, scenePos));
}

void Carta::finishLineSegment(const QPointF &scenePos)
{
    if (!m_lineDrawing)
    {
        return;
    }

    const QLineF finalLine(m_lineStartScenePos, scenePos);
    if (finalLine.length() < 2.0)
    {
        cancelLinePreview();
        return;
    }

    auto *lineItem = new QGraphicsLineItem(finalLine);
    QPen pen(m_drawingColor);
    QColor color = m_drawingColor;
    color.setAlphaF(std::clamp(m_strokeOpacity / 100.0, 0.0, 1.0));
    pen.setColor(color);
    pen.setWidth(std::max(1, m_strokeWidth));
    pen.setCapStyle(Qt::RoundCap);
    lineItem->setPen(pen);
    lineItem->setZValue(93.0);
    m_scene.addItem(lineItem);
    m_lineItems.append(lineItem);
    registerAnnotation(lineItem);
    cancelLinePreview();
}

void Carta::cancelLinePreview()
{
    if (!m_linePreview)
    {
        m_lineDrawing = false;
        return;
    }

    m_scene.removeItem(m_linePreview);
    delete m_linePreview;
    m_linePreview = nullptr;
    m_lineDrawing = false;
}

QGraphicsPathItem *Carta::addArcAnnotation(const QPointF &center, qreal radius, qreal startAngleDeg, qreal spanAngleDeg)
{
    if (radius <= 0.0 || qFuzzyIsNull(spanAngleDeg))
    {
        return nullptr;
    }

    QPainterPath path;
    const QRectF rect(center.x() - radius, center.y() - radius, radius * 2, radius * 2);
    path.arcMoveTo(rect, startAngleDeg);
    path.arcTo(rect, startAngleDeg, spanAngleDeg);

    auto *arcItem = new QGraphicsPathItem(path);
    QPen pen(m_drawingColor);
    pen.setWidth(std::max(1, m_strokeWidth));
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    QColor color = m_drawingColor;
    color.setAlphaF(std::clamp(m_strokeOpacity / 100.0, 0.0, 1.0));
    pen.setColor(color);
    arcItem->setPen(pen);
    arcItem->setZValue(91.0);
    m_scene.addItem(arcItem);
    m_arcItems.append(arcItem);
    registerAnnotation(arcItem);
    return arcItem;
}

void Carta::startStroke(const QPointF &scenePos)
{
    abortCurrentStroke();

    auto *pathItem = new QGraphicsPathItem();
    QPen pen(m_drawingColor);
    pen.setWidth(m_strokeWidth);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    QColor color = m_drawingColor;
    color.setAlphaF(std::clamp(m_strokeOpacity / 100.0, 0.0, 1.0));
    pen.setColor(color);
    pathItem->setPen(pen);
    pathItem->setZValue(90.0);

    m_currentStrokePath = QPainterPath(scenePos);
    m_currentStrokePath.lineTo(scenePos);
    pathItem->setPath(m_currentStrokePath);
    m_scene.addItem(pathItem);
    m_currentStroke = pathItem;
    m_painting = true;
}

void Carta::extendStroke(const QPointF &scenePos)
{
    if (!m_currentStroke)
    {
        return;
    }

    if (scenePos == m_currentStrokePath.currentPosition())
    {
        return;
    }

    m_currentStrokePath.lineTo(scenePos);
    m_currentStroke->setPath(m_currentStrokePath);
}

void Carta::finishStroke()
{
    if (!m_currentStroke)
    {
        m_painting = false;
        return;
    }

    m_strokeItems.append(m_currentStroke);
    registerAnnotation(m_currentStroke);
    m_currentStroke = nullptr;
    m_currentStrokePath = QPainterPath();
    m_painting = false;
}

void Carta::abortCurrentStroke()
{
    if (!m_currentStroke)
    {
        m_currentStrokePath = QPainterPath();
        m_painting = false;
        return;
    }

    m_scene.removeItem(m_currentStroke);
    delete m_currentStroke;
    m_currentStroke = nullptr;
    m_currentStrokePath = QPainterPath();
    m_painting = false;
}

void Carta::eraseAt(const QPointF &scenePos)
{
    const QList<QGraphicsItem *> items = m_scene.items(scenePos, Qt::IntersectsItemShape, Qt::DescendingOrder);
    for (QGraphicsItem *item : items)
    {
        if (removeAnnotationItem(item))
        {
            break;
        }
    }
}

bool Carta::removeAnnotationItem(QGraphicsItem *item)
{
    if (!item)
    {
        return false;
    }

    if (auto *textItem = qgraphicsitem_cast<QGraphicsSimpleTextItem *>(item))
    {
        if (!m_textItems.contains(textItem))
        {
            return false;
        }
        unregisterAnnotation(textItem);
        m_textItems.removeAll(textItem);
        m_scene.removeItem(textItem);
        delete textItem;
        return true;
    }

    if (auto *ellipseItem = qgraphicsitem_cast<QGraphicsEllipseItem *>(item))
    {
        if (!m_pointItems.contains(ellipseItem))
        {
            return false;
        }
        unregisterAnnotation(ellipseItem);
        m_pointItems.removeAll(ellipseItem);
        m_scene.removeItem(ellipseItem);
        delete ellipseItem;
        return true;
    }

    if (auto *lineItem = qgraphicsitem_cast<QGraphicsLineItem *>(item))
    {
        if (!m_lineItems.contains(lineItem))
        {
            return false;
        }
        unregisterAnnotation(lineItem);
        m_lineItems.removeAll(lineItem);
        m_scene.removeItem(lineItem);
        delete lineItem;
        return true;
    }

    if (auto *pathItem = qgraphicsitem_cast<QGraphicsPathItem *>(item))
    {
        if (m_strokeItems.contains(pathItem))
        {
            unregisterAnnotation(pathItem);
            m_strokeItems.removeAll(pathItem);
            m_scene.removeItem(pathItem);
            delete pathItem;
            return true;
        }

        if (m_arcItems.contains(pathItem))
        {
            unregisterAnnotation(pathItem);
            m_arcItems.removeAll(pathItem);
            m_scene.removeItem(pathItem);
            delete pathItem;
            return true;
        }

        return false;
    }

    return false;
}

void Carta::registerAnnotation(QGraphicsItem *item)
{
    if (!item)
    {
        return;
    }
    m_annotationStack.removeAll(item);
    m_annotationStack.append(item);
}

void Carta::unregisterAnnotation(QGraphicsItem *item)
{
    if (!item)
    {
        return;
    }
    m_annotationStack.removeAll(item);
}

void Carta::contextMenuEvent(QContextMenuEvent *event)
{
    if (!event)
    {
        QGraphicsView::contextMenuEvent(event);
        return;
    }

    QGraphicsItem *item = itemAt(event->pos());
    if (item && isAnnotationItem(item) && !isToolItem(item))
    {
        showAnnotationContextMenu(item, event->globalPos());
        event->accept();
        return;
    }

    QGraphicsView::contextMenuEvent(event);
}

void Carta::keyPressEvent(QKeyEvent *event)
{
    if (!event)
    {
        QGraphicsView::keyPressEvent(event);
        return;
    }

    if (event->key() == Qt::Key_Shift && !event->isAutoRepeat() && !m_shiftPressed)
    {
        m_shiftPressed = true;
        updateProjectionLines();
        event->accept();
        return;
    }

    QGraphicsView::keyPressEvent(event);
}

void Carta::keyReleaseEvent(QKeyEvent *event)
{
    if (!event)
    {
        QGraphicsView::keyReleaseEvent(event);
        return;
    }

    if (event->key() == Qt::Key_Shift && !event->isAutoRepeat() && m_shiftPressed)
    {
        m_shiftPressed = false;
        clearProjectionLines();
        event->accept();
        return;
    }

    QGraphicsView::keyReleaseEvent(event);
}

void Carta::focusInEvent(QFocusEvent *event)
{
    QGraphicsView::focusInEvent(event);
    if (QApplication::keyboardModifiers() & Qt::ShiftModifier)
    {
        if (!m_shiftPressed)
        {
            m_shiftPressed = true;
            updateProjectionLines();
        }
    }
}

void Carta::focusOutEvent(QFocusEvent *event)
{
    QGraphicsView::focusOutEvent(event);
    if (m_shiftPressed)
    {
        m_shiftPressed = false;
        clearProjectionLines();
    }
}

void Carta::showAnnotationContextMenu(QGraphicsItem *item, const QPoint &globalPos)
{
    if (!item)
    {
        return;
    }

    QMenu menu(this);
    QAction *changeColorAction = menu.addAction(tr("Cambiar color..."));

    QAction *selected = menu.exec(globalPos);
    if (selected == changeColorAction)
    {
        QColor currentColor = m_drawingColor;

        if (auto *textItem = qgraphicsitem_cast<QGraphicsSimpleTextItem *>(item))
        {
            currentColor = textItem->brush().color();
        }
        else if (auto *ellipseItem = qgraphicsitem_cast<QGraphicsEllipseItem *>(item))
        {
            currentColor = ellipseItem->brush().color();
        }
        else if (auto *lineItem = qgraphicsitem_cast<QGraphicsLineItem *>(item))
        {
            currentColor = lineItem->pen().color();
        }
        else if (auto *pathItem = qgraphicsitem_cast<QGraphicsPathItem *>(item))
        {
            currentColor = pathItem->pen().color();
        }

        const QColor newColor = QColorDialog::getColor(currentColor, this, tr("Seleccionar color"),
                                                        QColorDialog::ShowAlphaChannel);
        if (newColor.isValid())
        {
            changeAnnotationColor(item, newColor);
        }
    }
}

void Carta::changeAnnotationColor(QGraphicsItem *item, const QColor &newColor)
{
    if (!item || !newColor.isValid())
    {
        return;
    }

    if (auto *textItem = qgraphicsitem_cast<QGraphicsSimpleTextItem *>(item))
    {
        textItem->setBrush(QBrush(newColor));
        return;
    }

    if (auto *ellipseItem = qgraphicsitem_cast<QGraphicsEllipseItem *>(item))
    {
        QColor fillColor = newColor;
        ellipseItem->setBrush(QBrush(fillColor));
        QPen pen = ellipseItem->pen();
        pen.setColor(fillColor.darker(150));
        ellipseItem->setPen(pen);
        return;
    }

    if (auto *lineItem = qgraphicsitem_cast<QGraphicsLineItem *>(item))
    {
        QPen pen = lineItem->pen();
        pen.setColor(newColor);
        lineItem->setPen(pen);
        return;
    }

    if (auto *pathItem = qgraphicsitem_cast<QGraphicsPathItem *>(item))
    {
        QPen pen = pathItem->pen();
        pen.setColor(newColor);
        pathItem->setPen(pen);
        return;
    }
}

bool Carta::isAnnotationItem(QGraphicsItem *item) const
{
    if (!item)
    {
        return false;
    }

    if (qgraphicsitem_cast<QGraphicsSimpleTextItem *>(item))
    {
        return m_textItems.contains(static_cast<QGraphicsSimpleTextItem *>(item));
    }

    if (qgraphicsitem_cast<QGraphicsEllipseItem *>(item))
    {
        return m_pointItems.contains(static_cast<QGraphicsEllipseItem *>(item));
    }

    if (qgraphicsitem_cast<QGraphicsLineItem *>(item))
    {
        return m_lineItems.contains(static_cast<QGraphicsLineItem *>(item));
    }

    if (qgraphicsitem_cast<QGraphicsPathItem *>(item))
    {
        QGraphicsPathItem *pathItem = static_cast<QGraphicsPathItem *>(item);
        return m_strokeItems.contains(pathItem) || m_arcItems.contains(pathItem);
    }

    return false;
}

void Carta::updateProjectionLines()
{
    clearProjectionLines();

    if (!m_mapItem || !scene())
    {
        return;
    }

    const QRectF sceneRect = scene()->sceneRect();
    if (!sceneRect.isValid())
    {
        return;
    }

    for (QGraphicsEllipseItem *pointItem : m_pointItems)
    {
        if (!pointItem)
        {
            continue;
        }

        const QPointF center = pointItem->scenePos();

        auto *hLine = new QGraphicsLineItem(sceneRect.left(), center.y(), sceneRect.right(), center.y());
        QPen pen(Qt::white);
        pen.setWidth(1);
        pen.setStyle(Qt::DashLine);
        QColor color = Qt::white;
        color.setAlpha(120);
        pen.setColor(color);
        hLine->setPen(pen);
        hLine->setZValue(85.0);
        m_scene.addItem(hLine);
        m_projectionLines.append(hLine);

        auto *vLine = new QGraphicsLineItem(center.x(), sceneRect.top(), center.x(), sceneRect.bottom());
        vLine->setPen(pen);
        vLine->setZValue(85.0);
        m_scene.addItem(vLine);
        m_projectionLines.append(vLine);
    }
}

void Carta::clearProjectionLines()
{
    for (QGraphicsLineItem *line : m_projectionLines)
    {
        if (!line)
        {
            continue;
        }
        m_scene.removeItem(line);
        delete line;
    }
    m_projectionLines.clear();
}

