#include "carta.h"
#include "maptooltypes.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFrame>
#include <QGraphicsItem>
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
        showAngleLabel();
        event->accept();
    }

private:
    QString m_toolId;
    QPointer<Carta> m_view;
    qreal m_rotationDeg = 0.0;
    QGraphicsSimpleTextItem *m_angleLabel = nullptr;
    QGraphicsRectItem *m_angleLabelBg = nullptr;
    QTimer m_hideLabelTimer;

    void showAngleLabel()
    {
        if (!m_angleLabel || !m_angleLabelBg)
        {
            return;
        }

        const QString text = QString::number(qRound(m_rotationDeg)) + QStringLiteral("°");
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
    clearToolInstances();
    removeTextItems();
    removeStrokeItems();
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
    clearToolInstances();
    removeTextItems();
    removeStrokeItems();
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

    auto *item = new MapToolItem(toolId, resourcePath, this);
    if (!item->renderer() || !item->renderer()->isValid())
    {
        delete item;
        return false;
    }

    const QRectF bounds = item->boundingRect();
    item->setTransformOriginPoint(bounds.center());
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
    font.setPointSize(26);
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

    if (auto *pathItem = qgraphicsitem_cast<QGraphicsPathItem *>(item))
    {
        if (!m_strokeItems.contains(pathItem))
        {
            return false;
        }
        unregisterAnnotation(pathItem);
        m_strokeItems.removeAll(pathItem);
        m_scene.removeItem(pathItem);
        delete pathItem;
        return true;
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
