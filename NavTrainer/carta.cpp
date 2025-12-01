#include "carta.h"

#include <QFrame>
#include <QGraphicsPixmapItem>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSizePolicy>
#include <QWheelEvent>
#include <QtMath>

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
    m_scene.setBackgroundBrush(Qt::black);
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
    return true;
}

void Carta::clearMap()
{
    if (!m_mapItem)
    {
        return;
    }

    m_scene.removeItem(m_mapItem);
    delete m_mapItem;
    m_mapItem = nullptr;
    m_scene.setSceneRect({});
    resetTransform();
    m_baseScale = 1.0;
    m_currentScale = 1.0;
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
}

void Carta::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_mapItem)
    {
        m_panning = true;
        m_lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void Carta::mouseMoveEvent(QMouseEvent *event)
{
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
    if (event->button() == Qt::LeftButton && m_panning)
    {
        m_panning = false;
        unsetCursor();
        event->accept();
        return;
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
}

qreal Carta::minAllowedScale() const
{
    return m_baseScale * m_minZoomRatio;
}

qreal Carta::maxAllowedScale() const
{
    return m_baseScale * m_maxZoomRatio;
}
