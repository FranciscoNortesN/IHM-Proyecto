#include "carta.h"
#include "mapoverlaypanel.h"
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
#include <QTransform>
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
    static constexpr qreal kToolSceneScale = 1.0 / 6.0 * 1.5;

    MapToolItem(const QString &toolId, const QString &resourcePath, Carta *view)
        : QGraphicsSvgItem(resourcePath), m_toolId(toolId), m_view(view)
    {
        setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable |
                 QGraphicsItem::ItemSendsGeometryChanges);
        if (toolId == QLatin1String("tool_ruler") || toolId == QLatin1String("tool_compass")) {
            setScale(3.0 * 1.5);
        } else {
            setScale(kToolSceneScale);
        }
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
        m_angleLabel->hide();

        m_angleLabelBg = new QGraphicsRectItem(QRectF(0, 0, 0, 0), this);
        m_angleLabelBg->setBrush(QColor(0, 0, 0, 180));
        m_angleLabelBg->setPen(Qt::NoPen);
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
        // If this is the protractor and we are about to rotate, change cursor
        if (m_toolId == QLatin1String("tool_ruler")) {
            setCursor(Qt::SizeAllCursor); // or another cursor indicating rotation
        }
        QGraphicsSvgItem::mousePressEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override
    {
        QGraphicsSvgItem::mouseReleaseEvent(event);
        if (m_toolId == QLatin1String("tool_ruler")) {
            unsetCursor();
        }
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

        // Accept wheel anywhere on the tool, not just center
        const int deltaValue = event->delta();
        if (deltaValue == 0)
        {
            QGraphicsSvgItem::wheelEvent(event);
            return;
        }

        // Invert the rotation direction for more intuitive control
        const qreal degrees = -static_cast<qreal>(deltaValue) / 8.0;
        m_rotationDeg = std::fmod(m_rotationDeg + degrees, 360.0);
        if (m_rotationDeg < 0)
        {
            m_rotationDeg += 360.0;
        }

        setRotation(m_rotationDeg);
        
        // Force immediate visual update
        update();
        if (scene())
        {
            scene()->update();
        }
        
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
        Q_UNUSED(resourcePath);
        setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
        setAcceptHoverEvents(true);  // Enable hover for cursor feedback
        setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
        setCursor(Qt::ArrowCursor);
        
        // Override the base class scale - compass needs to be larger
        setScale(1.0 * 1.5);
        
        // Initialize compass geometry - larger for better visibility
        m_legLength = 180.0;
        m_legWidth = 12.0;
        m_hingeRadius = 18.0;
        m_pivotRotationDeg = -60.0;  // Pivot leg angle
        m_pencilRotationDeg = -120.0; // Pencil leg angle (forms a V shape)
    }

    ~CompassToolItem() override
    {
        discardArcPreview();
    }

public:
    // Public wrappers so outer code can forward events safely
    void handleMousePress(QGraphicsSceneMouseEvent *event) { mousePressEvent(event); }
    void handleMouseMove(QGraphicsSceneMouseEvent *event) { mouseMoveEvent(event); }
    void handleMouseRelease(QGraphicsSceneMouseEvent *event) { mouseReleaseEvent(event); }
    QRectF compassBoundingRect() const { return boundingRect(); }

protected:
    QRectF boundingRect() const override
    {
        const qreal r = m_legLength + m_hingeRadius + 20.0;
        return QRectF(-r, -r, r * 2.0, r * 2.0);
    }

    QPainterPath shape() const override
    {
        QPainterPath path;
        // Hinge area
        path.addEllipse(QPointF(0, 0), m_hingeRadius + 5, m_hingeRadius + 5);
        
        // Pivot leg
        QTransform pivotRot;
        pivotRot.rotate(m_pivotRotationDeg);
        QPolygonF pivotPoly;
        pivotPoly << QPointF(0, -m_legWidth/2) << QPointF(m_legLength, -2) 
                  << QPointF(m_legLength + 8, 0) << QPointF(m_legLength, 2) 
                  << QPointF(0, m_legWidth/2);
        path.addPolygon(pivotRot.map(pivotPoly));
        
        // Pencil leg
        QTransform pencilRot;
        pencilRot.rotate(m_pencilRotationDeg);
        QPolygonF pencilPoly;
        pencilPoly << QPointF(0, -m_legWidth/2) << QPointF(m_legLength - 10, -m_legWidth/2)
                   << QPointF(m_legLength - 10, -m_legWidth) << QPointF(m_legLength + 5, 0)
                   << QPointF(m_legLength - 10, m_legWidth) << QPointF(m_legLength - 10, m_legWidth/2)
                   << QPointF(0, m_legWidth/2);
        path.addPolygon(pencilRot.map(pencilPoly));
        
        return path;
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override
    {
        painter->setRenderHint(QPainter::Antialiasing, true);
        
        // Colors
        const QColor metalLight(200, 200, 210);
        const QColor metalDark(120, 120, 130);
        const QColor metalMid(160, 160, 170);
        const QColor pivotTip(80, 80, 90);
        const QColor pencilTip(60, 60, 60);
        const QColor pencilLead(30, 30, 30);
        const QColor hingeColor(140, 140, 150);
        const QColor hingeBorder(80, 80, 90);
        
        // Draw pivot leg (the one with sharp point)
        painter->save();
        painter->rotate(m_pivotRotationDeg);
        drawPivotLeg(painter, metalLight, metalDark, metalMid, pivotTip);
        painter->restore();
        
        // Draw pencil leg
        painter->save();
        painter->rotate(m_pencilRotationDeg);
        drawPencilLeg(painter, metalLight, metalDark, metalMid, pencilTip, pencilLead);
        painter->restore();
        
        // Draw central hinge (on top)
        drawHinge(painter, hingeColor, hingeBorder, metalLight);
    }
    
    void drawPivotLeg(QPainter *painter, const QColor &light, const QColor &dark, 
                      const QColor &mid, const QColor &tipColor)
    {
        // Main leg body with gradient
        QLinearGradient legGrad(0, -m_legWidth/2, 0, m_legWidth/2);
        legGrad.setColorAt(0, light);
        legGrad.setColorAt(0.3, mid);
        legGrad.setColorAt(0.7, mid);
        legGrad.setColorAt(1, dark);
        
        QPainterPath legPath;
        legPath.moveTo(m_hingeRadius - 2, -m_legWidth/2);
        legPath.lineTo(m_legLength - 15, -m_legWidth/2 + 1);
        legPath.lineTo(m_legLength - 15, -m_legWidth/2 - 1);
        legPath.lineTo(m_legLength + 10, 0);  // Sharp point
        legPath.lineTo(m_legLength - 15, m_legWidth/2 + 1);
        legPath.lineTo(m_legLength - 15, m_legWidth/2 - 1);
        legPath.lineTo(m_hingeRadius - 2, m_legWidth/2);
        legPath.closeSubpath();
        
        painter->setPen(QPen(dark, 1));
        painter->setBrush(legGrad);
        painter->drawPath(legPath);
        
        // Sharp metal tip
        QPainterPath tipPath;
        tipPath.moveTo(m_legLength - 15, -3);
        tipPath.lineTo(m_legLength + 10, 0);
        tipPath.lineTo(m_legLength - 15, 3);
        tipPath.closeSubpath();
        painter->setBrush(tipColor);
        painter->drawPath(tipPath);
        
        // Decorative lines on leg
        painter->setPen(QPen(dark.darker(110), 0.5));
        painter->drawLine(QPointF(m_hingeRadius + 10, 0), QPointF(m_legLength - 25, 0));
    }
    
    void drawPencilLeg(QPainter *painter, const QColor &light, const QColor &dark,
                       const QColor &mid, const QColor &tipColor, const QColor &leadColor)
    {
        // Main leg body
        QLinearGradient legGrad(0, -m_legWidth/2, 0, m_legWidth/2);
        legGrad.setColorAt(0, light);
        legGrad.setColorAt(0.3, mid);
        legGrad.setColorAt(0.7, mid);
        legGrad.setColorAt(1, dark);
        
        QPainterPath legPath;
        legPath.moveTo(m_hingeRadius - 2, -m_legWidth/2);
        legPath.lineTo(m_legLength - 20, -m_legWidth/2);
        legPath.lineTo(m_legLength - 20, -m_legWidth/2 - 3);  // Pencil holder wider
        legPath.lineTo(m_legLength - 5, -2);
        legPath.lineTo(m_legLength - 5, 2);
        legPath.lineTo(m_legLength - 20, m_legWidth/2 + 3);
        legPath.lineTo(m_legLength - 20, m_legWidth/2);
        legPath.lineTo(m_hingeRadius - 2, m_legWidth/2);
        legPath.closeSubpath();
        
        painter->setPen(QPen(dark, 1));
        painter->setBrush(legGrad);
        painter->drawPath(legPath);
        
        // Pencil holder
        QRectF holderRect(m_legLength - 22, -m_legWidth/2 - 4, 18, m_legWidth + 8);
        painter->setBrush(tipColor);
        painter->drawRoundedRect(holderRect, 2, 2);
        
        // Pencil lead tip
        QPainterPath leadPath;
        leadPath.moveTo(m_legLength - 5, -2);
        leadPath.lineTo(m_legLength + 5, 0);  // Lead tip
        leadPath.lineTo(m_legLength - 5, 2);
        leadPath.closeSubpath();
        painter->setBrush(leadColor);
        painter->setPen(QPen(leadColor.darker(), 0.5));
        painter->drawPath(leadPath);
        
        // Decorative lines
        painter->setPen(QPen(dark.darker(110), 0.5));
        painter->drawLine(QPointF(m_hingeRadius + 10, 0), QPointF(m_legLength - 30, 0));
    }
    
    void drawHinge(QPainter *painter, const QColor &base, const QColor &border, const QColor &highlight)
    {
        // Outer hinge ring
        QRadialGradient hingeGrad(0, 0, m_hingeRadius);
        hingeGrad.setColorAt(0, highlight);
        hingeGrad.setColorAt(0.5, base);
        hingeGrad.setColorAt(1, border);
        
        painter->setPen(QPen(border.darker(), 1.5));
        painter->setBrush(hingeGrad);
        painter->drawEllipse(QPointF(0, 0), m_hingeRadius, m_hingeRadius);
        
        // Inner hinge screw
        QRadialGradient screwGrad(0, -2, m_hingeRadius * 0.5);
        screwGrad.setColorAt(0, highlight.lighter(120));
        screwGrad.setColorAt(1, base.darker(110));
        
        painter->setBrush(screwGrad);
        painter->drawEllipse(QPointF(0, 0), m_hingeRadius * 0.6, m_hingeRadius * 0.6);
        
        // Screw slot
        painter->setPen(QPen(border.darker(130), 2));
        painter->drawLine(QPointF(-m_hingeRadius * 0.35, 0), QPointF(m_hingeRadius * 0.35, 0));
    }

    void wheelEvent(QGraphicsSceneWheelEvent *event) override
    {
        if (!event)
        {
            return;
        }

        const int deltaValue = event->delta();
        if (deltaValue == 0)
        {
            return;
        }

        // Adjust the spread between legs
        const qreal step = static_cast<qreal>(deltaValue) / 120.0 * 3.0;
        qreal currentSpread = m_pivotRotationDeg - m_pencilRotationDeg;
        currentSpread = std::clamp(currentSpread + step, kMinSpreadDeg, kMaxSpreadDeg);
        m_pencilRotationDeg = m_pivotRotationDeg - currentSpread;
        
        update();
        if (scene()) scene()->update();
        event->accept();
    }

    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override
    {
        if (!event) return;
        
        const QPointF pos = event->pos();
        const QPointF pivotTip = pivotTipLocal();
        const QPointF pencilTip = pencilTipLocal();
        
        const qreal distToPencil = QLineF(pos, pencilTip).length();
        const qreal distToPivot = QLineF(pos, pivotTip).length();
        const qreal distToHinge = QLineF(pos, QPointF(0, 0)).length();
        
        if (distToPencil <= 40.0)
        {
            setCursor(Qt::CrossCursor);  // Pencil tip - for drawing
        }
        else if (distToPivot <= 40.0)
        {
            setCursor(Qt::PointingHandCursor);  // Pivot tip - for rotating
        }
        else if (distToHinge <= m_hingeRadius)
        {
            setCursor(Qt::SizeAllCursor);  // Hinge - for moving
        }
        else
        {
            setCursor(Qt::ArrowCursor);
        }
    }

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (!event)
        {
            MapToolItem::mousePressEvent(event);
            return;
        }

        const QPointF pos = event->pos();
        const QPointF pivotTip = pivotTipLocal();
        const QPointF pencilTip = pencilTipLocal();

        const qreal distToPencil = QLineF(pos, pencilTip).length();
        const qreal distToPivot = QLineF(pos, pivotTip).length();
        const qreal distToHinge = QLineF(pos, QPointF(0, 0)).length();

        // Check tips FIRST - they have priority over hinge
        // Use larger threshold for tips (40 pixels)
        if (distToPencil <= 40.0)
        {
            // Drag pencil leg - left click draws arc, right click just adjusts spread
            beginArcDraw(event, event->button() == Qt::LeftButton);
            event->accept();
            return;
        }

        if (distToPivot <= 40.0)
        {
            // Drag from pivot tip - rotate whole compass
            beginRotateWhole(event);
            event->accept();
            return;
        }

        // Hinge area is ONLY the small center circle
        if (distToHinge <= m_hingeRadius)
        {
            // Drag from hinge to move whole compass
            beginMove(event);
            event->accept();
            return;
        }

        MapToolItem::mousePressEvent(event);
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (m_drawingArc)
        {
            updateArcDraw(event);
            event->accept();
            return;
        }

        if (m_rotatingWhole)
        {
            updateRotateWhole(event);
            event->accept();
            return;
        }

        if (m_draggingMove)
        {
            updateMove(event);
            event->accept();
            return;
        }

        MapToolItem::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (m_drawingArc)
        {
            finalizeArcDraw();
            unsetCursor();
            event->accept();
            return;
        }

        if (m_rotatingWhole)
        {
            m_rotatingWhole = false;
            unsetCursor();
            event->accept();
            return;
        }

        if (m_draggingMove)
        {
            m_draggingMove = false;
            if (m_view) m_view->handleToolDragFinished(this);
            event->accept();
            return;
        }

        MapToolItem::mouseReleaseEvent(event);
    }

private:
    static constexpr qreal kMinSpreadDeg = 15.0;
    static constexpr qreal kMaxSpreadDeg = 160.0;

    qreal m_legLength = 180.0;
    qreal m_legWidth = 12.0;
    qreal m_hingeRadius = 18.0;
    qreal m_pivotRotationDeg = -60.0;
    qreal m_pencilRotationDeg = -120.0;

    // Interaction states
    bool m_draggingMove = false;
    bool m_rotatingWhole = false;
    bool m_drawingArc = false;
    bool m_shouldPaint = false;
    
    QPointF m_moveSceneOffset;
    qreal m_rotateStartAngle = 0.0;
    qreal m_rotateStartPivot = 0.0;
    qreal m_rotateStartPencil = 0.0;

    // Arc drawing state
    QPointF m_arcCenterMapScene;  // Pivot tip position in map scene coords
    qreal m_arcStartAngle = 0.0;
    qreal m_arcCurrentAngle = 0.0;
    qreal m_arcRadius = 0.0;
    QGraphicsPathItem *m_arcPreview = nullptr;

    QPointF pivotTipLocal() const
    {
        const QTransform rot = QTransform().rotate(m_pivotRotationDeg);
        return rot.map(QPointF(m_legLength + 10, 0));
    }

    QPointF pencilTipLocal() const
    {
        const QTransform rot = QTransform().rotate(m_pencilRotationDeg);
        return rot.map(QPointF(m_legLength + 5, 0));
    }

    void beginMove(QGraphicsSceneMouseEvent *event)
    {
        if (!event) return;
        m_draggingMove = true;
        if (m_view) m_view->handleToolDragStarted();
        m_moveSceneOffset = event->scenePos() - pos();
    }

    void updateMove(QGraphicsSceneMouseEvent *event)
    {
        if (!event) return;
        setPos(event->scenePos() - m_moveSceneOffset);
    }

    void beginRotateWhole(QGraphicsSceneMouseEvent *event)
    {
        if (!event) return;
        m_rotatingWhole = true;
        m_rotateStartAngle = QLineF(QPointF(0, 0), event->pos()).angle();
        m_rotateStartPivot = m_pivotRotationDeg;
        m_rotateStartPencil = m_pencilRotationDeg;
    }

    void updateRotateWhole(QGraphicsSceneMouseEvent *event)
    {
        if (!event) return;
        qreal currentAngle = QLineF(QPointF(0, 0), event->pos()).angle();
        qreal delta = currentAngle - m_rotateStartAngle;
        m_pivotRotationDeg = m_rotateStartPivot - delta;
        m_pencilRotationDeg = m_rotateStartPencil - delta;
        update();
        if (scene()) scene()->update();
    }

    void beginArcDraw(QGraphicsSceneMouseEvent *event, bool shouldPaint)
    {
        if (!event || !m_view) return;

        m_drawingArc = true;
        m_shouldPaint = shouldPaint;
        
        // Get the pivot tip position in MAP scene coordinates
        // Tool scene pos -> viewport pos -> map scene pos
        QPointF pivotToolScene = mapToScene(pivotTipLocal());
        QPoint viewportPos = pivotToolScene.toPoint();
        m_arcCenterMapScene = m_view->mapToScene(viewportPos);
        
        // Get pencil tip in map scene for initial angle
        QPointF pencilToolScene = mapToScene(pencilTipLocal());
        QPoint pencilViewport = pencilToolScene.toPoint();
        QPointF pencilMapScene = m_view->mapToScene(pencilViewport);
        
        // Calculate radius and start angle in map scene
        m_arcRadius = QLineF(m_arcCenterMapScene, pencilMapScene).length();
        m_arcStartAngle = QLineF(m_arcCenterMapScene, pencilMapScene).angle();
        m_arcCurrentAngle = m_arcStartAngle;
        
        if (m_shouldPaint)
        {
            createArcPreview();
        }
    }

    void updateArcDraw(QGraphicsSceneMouseEvent *event)
    {
        if (!event || !m_view) return;

        // Update pencil leg angle based on mouse position (in local coords)
        qreal mouseAngle = QLineF(QPointF(0, 0), event->pos()).angle();
        qreal newPencilRot = -mouseAngle;
        
        // Clamp spread between legs
        qreal spread = m_pivotRotationDeg - newPencilRot;
        spread = std::clamp(spread, kMinSpreadDeg, kMaxSpreadDeg);
        m_pencilRotationDeg = m_pivotRotationDeg - spread;
        
        // Get new pencil tip in map scene
        QPointF pencilToolScene = mapToScene(pencilTipLocal());
        QPoint pencilViewport = pencilToolScene.toPoint();
        QPointF pencilMapScene = m_view->mapToScene(pencilViewport);
        
        // Update current angle for arc
        m_arcCurrentAngle = QLineF(m_arcCenterMapScene, pencilMapScene).angle();
        m_arcRadius = QLineF(m_arcCenterMapScene, pencilMapScene).length();
        
        if (m_shouldPaint && m_arcPreview)
        {
            updateArcPreviewPath();
        }
        
        update();
        if (scene()) scene()->update();
    }

    void finalizeArcDraw()
    {
        if (m_shouldPaint && m_arcPreview)
        {
            qreal span = m_arcCurrentAngle - m_arcStartAngle;
            // Normalize span
            while (span > 180) span -= 360;
            while (span < -180) span += 360;
            
            if (std::abs(span) >= 2.0 && m_view)
            {
                m_view->addArcAnnotation(m_arcCenterMapScene, m_arcRadius, m_arcStartAngle, span);
            }
        }
        
        discardArcPreview();
        m_drawingArc = false;
        m_shouldPaint = false;
    }

    void createArcPreview()
    {
        if (m_arcPreview || !m_view || !m_view->scene()) return;

        m_arcPreview = new QGraphicsPathItem();
        QPen pen(m_view->drawingColor());
        pen.setWidth(std::max(2, m_view->strokeWidth()));
        pen.setStyle(Qt::SolidLine);
        pen.setCapStyle(Qt::RoundCap);
        m_arcPreview->setPen(pen);
        m_arcPreview->setZValue(88.0);
        m_view->scene()->addItem(m_arcPreview);
    }

    void updateArcPreviewPath()
    {
        if (!m_arcPreview) return;

        qreal span = m_arcCurrentAngle - m_arcStartAngle;
        while (span > 180) span -= 360;
        while (span < -180) span += 360;

        QPainterPath path;
        QRectF arcRect(m_arcCenterMapScene.x() - m_arcRadius,
                       m_arcCenterMapScene.y() - m_arcRadius,
                       m_arcRadius * 2, m_arcRadius * 2);
        path.arcMoveTo(arcRect, m_arcStartAngle);
        path.arcTo(arcRect, m_arcStartAngle, span);
        m_arcPreview->setPath(path);
    }

    void discardArcPreview()
    {
        if (!m_arcPreview) return;
        if (m_view && m_view->scene())
        {
            m_view->scene()->removeItem(m_arcPreview);
        }
        delete m_arcPreview;
        m_arcPreview = nullptr;
    }
};

// -------------------------------------------------------
// RulerToolItem - Custom ruler with two handlebars
// One handlebar moves the ruler, the other rotates with anchor at the opposite end
// -------------------------------------------------------
class RulerToolItem : public MapToolItem
{
public:
    RulerToolItem(const QString &toolId, const QString &resourcePath, Carta *view)
        : MapToolItem(toolId, resourcePath, view)
    {
        // IMPORTANT: Disable automatic moving - we handle it manually via handlebars
        setFlag(QGraphicsItem::ItemIsMovable, false);
        
        setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
        setAcceptHoverEvents(true);
        setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
        setCursor(Qt::ArrowCursor);
        
        // Override scale - ruler SVG is very large (4872 x 348)
        // Scale to a reasonable size: about 400 pixels wide
        setScale((600.0 / 4872.0) * 1.5);
    }

    ~RulerToolItem() override = default;

    // Public methods for event forwarding from Carta
    void handleMousePress(QGraphicsSceneMouseEvent *event) { mousePressEvent(event); }
    void handleMouseMove(QGraphicsSceneMouseEvent *event) { mouseMoveEvent(event); }
    void handleMouseRelease(QGraphicsSceneMouseEvent *event) { mouseReleaseEvent(event); }
    QRectF rulerBoundingRect() const { return MapToolItem::boundingRect(); }

protected:
    // Handlebar dimensions (in local SVG coordinates)
    static constexpr qreal kHandleRadius = 80.0;  // Radius of circular handles
    static constexpr qreal kHandleOffset = 50.0;  // Distance from edge
    
    // Get the bounding rect of the SVG
    QRectF svgRect() const
    {
        // The ruler SVG is 4872.2583 x 348.90323
        return QRectF(0, 0, 4872.2583, 348.90323);
    }
    
    // Left end position (anchor for rotation) - in local coords
    QPointF leftEndCenter() const
    {
        QRectF rect = svgRect();
        return QPointF(kHandleOffset + kHandleRadius, rect.height() / 2);
    }
    
    // Right handlebar position (ROTATE handle) - in local coords
    QPointF rightHandleCenter() const
    {
        QRectF rect = svgRect();
        return QPointF(rect.width() - kHandleOffset - kHandleRadius, rect.height() / 2);
    }
    
    // Check if point is in right (rotate) handle
    bool isInRightHandle(const QPointF &localPos) const
    {
        return QLineF(localPos, rightHandleCenter()).length() <= kHandleRadius * 1.5;
    }

    QRectF boundingRect() const override
    {
        // Include extra space for the handles
        QRectF base = MapToolItem::boundingRect();
        return base.adjusted(-kHandleRadius, -kHandleRadius, kHandleRadius, kHandleRadius);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override
    {
        // Draw the SVG ruler first
        MapToolItem::paint(painter, option, widget);
        
        painter->save();
        
        // Draw right handle (ROTATE) - green circle with rotation icon
        QPointF rightCenter = rightHandleCenter();
        painter->setPen(QPen(Qt::white, 4));
        painter->setBrush(QColor(0, 150, 80, 200));
        painter->drawEllipse(rightCenter, kHandleRadius, kHandleRadius);
        
        // Draw rotation arc icon
        painter->setPen(QPen(Qt::white, 6, Qt::SolidLine, Qt::RoundCap));
        qreal arrowLen = kHandleRadius * 0.5;
        QRectF arcRect(rightCenter.x() - arrowLen, rightCenter.y() - arrowLen, arrowLen * 2, arrowLen * 2);
        painter->drawArc(arcRect, 30 * 16, 240 * 16);  // Draw arc
        // Arrow at end of arc
        painter->drawLine(rightCenter + QPointF(arrowLen * 0.5, -arrowLen * 0.87),
                          rightCenter + QPointF(arrowLen * 0.3, -arrowLen * 0.5));
        painter->drawLine(rightCenter + QPointF(arrowLen * 0.5, -arrowLen * 0.87),
                          rightCenter + QPointF(arrowLen * 0.8, -arrowLen * 0.7));
        
        painter->restore();
    }

    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override
    {
        if (!event) return;
        
        const QPointF pos = event->pos();
        
        if (isInRightHandle(pos))
        {
            setCursor(Qt::PointingHandCursor);  // Rotate cursor
        }
        else
        {
            setCursor(Qt::SizeAllCursor);  // Move cursor for anywhere else
        }
    }

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (!event)
        {
            MapToolItem::mousePressEvent(event);
            return;
        }

        const QPointF pos = event->pos();

        if (isInRightHandle(pos))
        {
            // Right handle: ROTATE around left end (anchor point)
            setCursor(Qt::ClosedHandCursor); // Show holding cursor
            beginRotate(event);
            event->accept();
            return;
        }

        // Click anywhere else on the ruler: MOVE
        beginMove(event);
        event->accept();
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (!event)
        {
            MapToolItem::mouseMoveEvent(event);
            return;
        }

        if (m_draggingMove)
        {
            updateMove(event);
            event->accept();
            return;
        }

        if (m_rotating)
        {
            updateRotate(event);
            event->accept();
            return;
        }

        MapToolItem::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (m_draggingMove)
        {
            m_draggingMove = false;
            if (m_view) m_view->handleToolDragFinished(this);
        }

        if (m_rotating)
        {
            m_rotating = false;
            unsetCursor(); // Restore cursor after rotation
            // Show final angle
            qreal angle = std::fmod(rotation(), 360.0);
            if (angle < 0) angle += 360.0;
            displayMeasurement(QString::number(angle, 'f', 1) + QStringLiteral("°"));
        }

        event->accept();
    }

    // Disable wheel rotation - now handled by handlebars
    void wheelEvent(QGraphicsSceneWheelEvent *event) override
    {
        // Just accept and ignore
        if (event) event->accept();
    }

private:
    bool m_draggingMove = false;
    bool m_rotating = false;
    QPointF m_moveSceneOffset;
    QPointF m_rotateAnchorScene;  // Anchor point in scene coords
    QPointF m_rotateStartPos;     // Starting position when rotation began
    qreal m_rotateStartAngle = 0.0;
    qreal m_rotateStartRotation = 0.0;

    void beginMove(QGraphicsSceneMouseEvent *event)
    {
        if (!event) return;
        m_draggingMove = true;
        if (m_view) m_view->handleToolDragStarted();
        m_moveSceneOffset = event->scenePos() - pos();
    }

    void updateMove(QGraphicsSceneMouseEvent *event)
    {
        if (!event) return;
        setPos(event->scenePos() - m_moveSceneOffset);
    }

    void beginRotate(QGraphicsSceneMouseEvent *event)
    {
        if (!event) return;
        m_rotating = true;
        
        // Store the left end position in scene coords BEFORE rotation
        m_rotateAnchorScene = mapToScene(leftEndCenter());
        
        // Store starting angle from anchor to mouse
        QPointF mouseScene = event->scenePos();
        m_rotateStartAngle = QLineF(m_rotateAnchorScene, mouseScene).angle();
        m_rotateStartRotation = rotation();
        m_rotateStartPos = pos();
        
        // Set transform origin to left end (in local coordinates)
        setTransformOriginPoint(leftEndCenter());
    }

    void updateRotate(QGraphicsSceneMouseEvent *event)
    {
        if (!event) return;
        
        // Calculate current angle from original anchor to mouse
        QPointF mouseScene = event->scenePos();
        qreal currentAngle = QLineF(m_rotateAnchorScene, mouseScene).angle();
        
        // Delta angle (Qt angles are counterclockwise, so negate)
        qreal delta = m_rotateStartAngle - currentAngle;
        
        // Apply rotation
        qreal newRotation = m_rotateStartRotation + delta;
        setRotation(newRotation);
        
        // After rotation, the left end has moved. We need to move the item
        // so that the left end stays at the original anchor position.
        QPointF newAnchorScene = mapToScene(leftEndCenter());
        QPointF correction = m_rotateAnchorScene - newAnchorScene;
        setPos(pos() + correction);
        
        update();
        if (scene()) scene()->update();
    }
};

Carta::Carta(QWidget *parent)
    : QGraphicsView(parent)
{
    setScene(&m_scene);
    m_toolScene.setSceneRect(-5000, -5000, 10000, 10000);
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
        viewport()->setAttribute(Qt::WA_AcceptDrops, true);
    }
    m_scene.setBackgroundBrush(Qt::black);
    
    // Ensure the view is ready for tool drops
    QTimer::singleShot(0, this, [this]() {
        viewport()->update();
    });
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
    m_overlayViewportPos = desired;
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
    // Check if wheel event is over a tool
    const QPoint viewPos = wheelEventViewportPos(event);
    const QPointF toolScenePos = QPointF(viewPos);
    QGraphicsItem *toolItem = m_toolScene.itemAt(toolScenePos, QTransform());
    
    if (toolItem)
    {
        // Find top-level MapToolItem
        while (toolItem && !qgraphicsitem_cast<MapToolItem*>(toolItem))
        {
            toolItem = toolItem->parentItem();
        }
        
        if (MapToolItem *mapTool = qgraphicsitem_cast<MapToolItem*>(toolItem))
        {
            QGraphicsSceneWheelEvent sceneEvent(QEvent::GraphicsSceneWheel);
            sceneEvent.setScenePos(toolScenePos);
            sceneEvent.setPos(mapTool->mapFromScene(toolScenePos));
            sceneEvent.setDelta(event->angleDelta().y());
            sceneEvent.setButtons(event->buttons());
            sceneEvent.setModifiers(event->modifiers());
            m_toolScene.sendEvent(mapTool, &sceneEvent);
            if (sceneEvent.isAccepted())
            {
                viewport()->update();
                event->accept();
                return;
            }
        }
    }

    if (!m_mapItem)
    {
        event->ignore();
        return;
    }

    // Only allow zoom if Shift is held
    if (!(event->modifiers() & Qt::ShiftModifier)) {
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

    // FIRST: Check if clicking on overlay (toolbox) - this takes priority
    if (m_overlayWidget && !m_overlayMouseTransparent)
    {
        const QPoint overlayPos = m_overlayWidget->mapFromParent(event->pos());
        if (m_overlayWidget->rect().contains(overlayPos))
        {
            // Pass to base class which will let the child widget handle it
            QGraphicsView::mousePressEvent(event);
            return;
        }
    }

    // SECOND: Check if clicking on a tool in tool scene (only if NOT over overlay)
    const QPointF toolScenePos = QPointF(event->pos());
    QGraphicsItem *toolItem = m_toolScene.itemAt(toolScenePos, QTransform());
    
    if (toolItem)
    {
        // Find top-level MapToolItem
        while (toolItem && !qgraphicsitem_cast<MapToolItem*>(toolItem))
        {
            toolItem = toolItem->parentItem();
        }
        
        if (MapToolItem *mapTool = qgraphicsitem_cast<MapToolItem*>(toolItem))
        {
            // Check if this is a RulerToolItem - let it handle its own events
            if (RulerToolItem *ruler = dynamic_cast<RulerToolItem*>(mapTool))
            {
                // Forward mouse event to the ruler's scene event handler
                QPointF localPos = ruler->mapFromScene(toolScenePos);
                QGraphicsSceneMouseEvent sceneEvent(QEvent::GraphicsSceneMousePress);
                sceneEvent.setPos(localPos);
                sceneEvent.setScenePos(toolScenePos);
                sceneEvent.setScreenPos(event->globalPosition().toPoint());
                sceneEvent.setButton(event->button());
                sceneEvent.setButtons(event->buttons());
                sceneEvent.setModifiers(event->modifiers());
                
                // Store reference for move/release events
                m_draggedToolItem = ruler;
                m_activeRuler = ruler;
                
                // Send to ruler
                ruler->handleMousePress(&sceneEvent);
                event->accept();
                return;
            }
            
            // If this is the compass, forward the event to let it handle rotate/paint
            if (CompassToolItem *compass = dynamic_cast<CompassToolItem*>(mapTool))
            {
                QPointF localPos = compass->mapFromScene(toolScenePos);
                QGraphicsSceneMouseEvent sceneEvent(QEvent::GraphicsSceneMousePress);
                sceneEvent.setPos(localPos);
                sceneEvent.setScenePos(toolScenePos);
                sceneEvent.setScreenPos(event->globalPosition().toPoint());
                sceneEvent.setButton(event->button());
                sceneEvent.setButtons(event->buttons());
                sceneEvent.setModifiers(event->modifiers());

                // Store reference for move/release events
                m_draggedToolItem = compass;
                m_activeCompass = compass;

                // Forward to compass via public wrapper
                compass->handleMousePress(&sceneEvent);
                event->accept();
                return;
            }

            // Other tools use default dragging
            m_draggedToolItem = mapTool;
            m_toolDragOffset = toolScenePos - mapTool->pos();
            event->accept();
            return;
        }
    }

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
    // Forward to RulerToolItem if we have an active ruler
    if (m_activeRuler)
    {
        const QPointF toolScenePos = QPointF(event->pos());
        QPointF localPos = m_activeRuler->mapFromScene(toolScenePos);
        
        QGraphicsSceneMouseEvent sceneEvent(QEvent::GraphicsSceneMouseMove);
        sceneEvent.setPos(localPos);
        sceneEvent.setScenePos(toolScenePos);
        sceneEvent.setScreenPos(event->globalPosition().toPoint());
        sceneEvent.setButton(event->button());
        sceneEvent.setButtons(event->buttons());
        sceneEvent.setModifiers(event->modifiers());
        
        m_activeRuler->handleMouseMove(&sceneEvent);
        viewport()->update();
        event->accept();
        return;
    }

    // Forward to CompassToolItem if we have an active compass
    if (m_activeCompass)
    {
        const QPointF toolScenePos = QPointF(event->pos());
        QPointF localPos = m_activeCompass->mapFromScene(toolScenePos);

        QGraphicsSceneMouseEvent sceneEvent(QEvent::GraphicsSceneMouseMove);
        sceneEvent.setPos(localPos);
        sceneEvent.setScenePos(toolScenePos);
        sceneEvent.setScreenPos(event->globalPosition().toPoint());
        sceneEvent.setButton(event->button());
        sceneEvent.setButtons(event->buttons());
        sceneEvent.setModifiers(event->modifiers());

        m_activeCompass->handleMouseMove(&sceneEvent);
        viewport()->update();
        event->accept();
        return;
    }
    
    // Forward to tool scene if we're dragging a tool (non-ruler/non-compass)
    if (m_draggedToolItem)
    {
        const QPointF toolScenePos = QPointF(event->pos());
        // Move the tool directly
        m_draggedToolItem->setPos(toolScenePos - m_toolDragOffset);
        viewport()->update();
        event->accept();
        return;
    }

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
    // Forward to RulerToolItem if we have an active ruler
    if (m_activeRuler)
    {
        const QPointF toolScenePos = QPointF(event->pos());
        QPointF localPos = m_activeRuler->mapFromScene(toolScenePos);
        
        QGraphicsSceneMouseEvent sceneEvent(QEvent::GraphicsSceneMouseRelease);
        sceneEvent.setPos(localPos);
        sceneEvent.setScenePos(toolScenePos);
        sceneEvent.setScreenPos(event->globalPosition().toPoint());
        sceneEvent.setButton(event->button());
        sceneEvent.setButtons(event->buttons());
        sceneEvent.setModifiers(event->modifiers());
        
        m_activeRuler->handleMouseRelease(&sceneEvent);
        
        // Store final position
        storeToolViewportPos(m_activeRuler);
        
        // Check if returned to toolbox
        const QRectF toolBounds = m_activeRuler->rulerBoundingRect();
        const QPoint centerPoint = QPoint(
            static_cast<int>(m_activeRuler->pos().x() + toolBounds.width() * m_activeRuler->scale() / 2),
            static_cast<int>(m_activeRuler->pos().y() + toolBounds.height() * m_activeRuler->scale() / 2));
        
        if (overlayContainsViewportPoint(centerPoint))
        {
            removeTool(m_activeRuler->toolId());
        }
        
        m_activeRuler = nullptr;
        m_draggedToolItem = nullptr;
        viewport()->update();
        event->accept();
        return;
    }

    // Forward to CompassToolItem if we have an active compass
    if (m_activeCompass)
    {
        const QPointF toolScenePos = QPointF(event->pos());
        QPointF localPos = m_activeCompass->mapFromScene(toolScenePos);

        QGraphicsSceneMouseEvent sceneEvent(QEvent::GraphicsSceneMouseRelease);
        sceneEvent.setPos(localPos);
        sceneEvent.setScenePos(toolScenePos);
        sceneEvent.setScreenPos(event->globalPosition().toPoint());
        sceneEvent.setButton(event->button());
        sceneEvent.setButtons(event->buttons());
        sceneEvent.setModifiers(event->modifiers());

        m_activeCompass->handleMouseRelease(&sceneEvent);

        // Store final position
        storeToolViewportPos(m_activeCompass);

        // Use bounding rect center for checking if it's over the toolbox
        const QRectF toolBounds = m_activeCompass->compassBoundingRect();
        const QPoint centerPoint = QPoint(
            static_cast<int>(m_activeCompass->pos().x() + toolBounds.width() * m_activeCompass->scale() / 2),
            static_cast<int>(m_activeCompass->pos().y() + toolBounds.height() * m_activeCompass->scale() / 2));

        if (overlayContainsViewportPoint(centerPoint))
        {
            removeTool(m_activeCompass->toolId());
        }

        m_activeCompass = nullptr;
        m_draggedToolItem = nullptr;
        viewport()->update();
        event->accept();
        return;
    }
    
    // Forward to tool scene if we were dragging a tool (non-ruler/non-compass)
    if (m_draggedToolItem)
    {
        // Store final position and check if returned to toolbox
        storeToolViewportPos(m_draggedToolItem);
        
        // Use tool center for checking if it's over the toolbox
        const QRectF toolBounds = m_draggedToolItem->boundingRect();
        const QPoint centerPoint = QPoint(static_cast<int>(m_draggedToolItem->pos().x() + toolBounds.width() * m_draggedToolItem->scale() / 2),
                                          static_cast<int>(m_draggedToolItem->pos().y() + toolBounds.height() * m_draggedToolItem->scale() / 2));
        
        if (overlayContainsViewportPoint(centerPoint))
        {
            removeTool(m_draggedToolItem->toolId());
        }
        
        m_draggedToolItem = nullptr;
        viewport()->update();
        event->accept();
        return;
    }

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

void Carta::enterEvent(QEnterEvent *event)
{
    QGraphicsView::enterEvent(event);
    setFocus();  // Grab keyboard focus when mouse enters
}

void Carta::leaveEvent(QEvent *event)
{
    QGraphicsView::leaveEvent(event);
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
    const QPointF toolScenePos = QPointF(widgetPos);

    if (addToolItem(toolId, resourcePath, toolScenePos))
    {
        // Select the newly added tool so it can be moved immediately
        if (MapToolItem *addedTool = m_activeToolItems.value(toolId))
        {
            addedTool->setSelected(true);
        }
        // Force viewport update so tool appears immediately
        viewport()->update();
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

    QPoint desired = m_overlayViewportPos;
    if (clampToViewport)
    {
        desired = clampOverlayToViewport(desired);
    }

    m_overlayViewportPos = desired;
    m_overlayWidget->move(desired);
}

QPoint Carta::clampOverlayToViewport(const QPoint &candidate) const
{
    if (!m_overlayWidget || !viewport())
    {
        return candidate;
    }

    int minVisibleHeight = m_overlayWidget->height();
    if (auto *panel = qobject_cast<MapOverlayPanel *>(m_overlayWidget))
    {
        minVisibleHeight = panel->minimumVisibleHeight();
    }

    const int visibleHeight = std::max(0, minVisibleHeight);
    const int maxX = std::max(0, viewport()->width() - m_overlayWidget->width());
    const int maxY = std::max(0, viewport()->height() - visibleHeight);
    QPoint clamped(candidate);
    clamped.setX(std::clamp(clamped.x(), 0, maxX));
    clamped.setY(std::clamp(clamped.y(), 0, maxY));
    return clamped;
}

void Carta::setOverlayDefaultScenePos()
{
    const QPoint defaultViewportPoint(m_overlayMargin, m_overlayMargin);
    m_overlayViewportPos = defaultViewportPoint;
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
        storeToolViewportPos(existing);
        return true;
    }

    MapToolItem *item = nullptr;
    if (toolId == QLatin1String("tool_compass"))
    {
        item = new CompassToolItem(toolId, resourcePath, this);
    }
    else if (toolId == QLatin1String("tool_ruler"))
    {
        item = new RulerToolItem(toolId, resourcePath, this);
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
    // scenePos is already in viewport coordinates for tool scene
    item->setPos(scenePos);

    m_toolScene.addItem(item);
    m_activeToolItems.insert(toolId, item);
    storeToolViewportPos(item);
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
            m_toolScene.removeItem(item);
            delete item;
        }
    }

    m_activeToolItems.clear();
    m_toolViewportPos.clear();
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
    m_toolDragInProgress = true;
    setOverlayMouseTransparent(true);
}

void Carta::handleToolDragFinished(MapToolItem *item)
{
    setOverlayMouseTransparent(false);

    if (!item)
    {
        m_toolDragInProgress = false;
        return;
    }

    m_toolDragInProgress = false;

    // Only remove tool if it's dragged back INTO the overlay area
    // Tools being repositioned elsewhere should stay
    const QPointF itemViewportPos = item->pos();
    if (overlayContainsViewportPoint(itemViewportPos.toPoint()))
    {
        removeTool(item->toolId());
        return;
    }

    storeToolViewportPos(item);
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
    m_toolViewportPos.remove(item);
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

    m_toolViewportPos.remove(item);

    m_toolScene.removeItem(item);
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

void Carta::storeToolViewportPos(MapToolItem *item)
{
    if (!item || !viewport())
    {
        return;
    }

    m_toolViewportPos.insert(item, item->pos());
}

void Carta::repositionToolsToViewport()
{
    if (m_toolDragInProgress || m_activeToolItems.isEmpty() || !viewport())
    {
        return;
    }

    const QRectF viewRect = viewport()->rect();

    for (MapToolItem *item : m_activeToolItems)
    {
        if (!item)
        {
            continue;
        }

        QPointF storedPos = m_toolViewportPos.value(item, item->pos());
        storedPos.setX(std::clamp(storedPos.x(), viewRect.left(), viewRect.right()));
        storedPos.setY(std::clamp(storedPos.y(), viewRect.top(), viewRect.bottom()));
        m_toolViewportPos.insert(item, storedPos);
        item->setPos(storedPos);
    }
}

void Carta::drawForeground(QPainter *painter, const QRectF &rect)
{
    Q_UNUSED(rect);
    
    // Render tool scene on top without any view transform
    painter->save();
    painter->resetTransform();
    const QRect viewRect = viewport()->rect();
    
    // Clip to exclude overlay area so tools don't render on top of toolbox
    if (m_overlayWidget && m_overlayWidget->isVisible())
    {
        QRegion clipRegion(viewRect);
        clipRegion -= m_overlayWidget->geometry();
        painter->setClipRegion(clipRegion);
    }
    
    m_toolScene.render(painter, viewRect, QRectF(viewRect));
    painter->restore();
    
    QGraphicsView::drawForeground(painter, rect);
}

bool Carta::overlayContainsViewportPoint(const QPoint &point) const
{
    if (!m_overlayWidget)
    {
        return false;
    }

    return m_overlayWidget->geometry().contains(point);
}

MapToolItem *Carta::rulerItem() const
{
    return m_activeToolItems.value(QStringLiteral("tool_ruler"), nullptr);
}

bool Carta::pointHitsRuler(const QPointF &scenePos, MapToolItem *ruler) const
{
    if (!ruler)
    {
        return false;
    }

    // Convert map scene pos to viewport pos, then to tool scene pos
    const QPoint viewportPos = mapFromScene(scenePos);
    const QPointF toolScenePos = QPointF(viewportPos);
    const QPointF localPos = ruler->mapFromScene(toolScenePos);
    return ruler->shape().contains(localPos);
}

QPointF Carta::rulerDirection(MapToolItem *ruler) const
{
    if (!ruler)
    {
        return QPointF(1.0, 0.0);
    }

    // Ruler rotation is in tool scene, direction follows Qt's clockwise rotation
    // In screen coordinates (Y down), rotation angle follows standard trigonometry
    const qreal angleDeg = ruler->rotation();
    const qreal angleRad = qDegreesToRadians(angleDeg);
    return QPointF(std::cos(angleRad), std::sin(angleRad));
}

QPointF Carta::applyRulerSnap(const QPointF &prevPoint, const QPointF &candidate) const
{
    MapToolItem *ruler = rulerItem();
    if (!pointHitsRuler(candidate, ruler))
    {
        return candidate;
    }

    const QPointF dir = rulerDirection(ruler);
    const QPointF delta = candidate - prevPoint;
    const qreal projection = delta.x() * dir.x() + delta.y() * dir.y();
    return prevPoint + dir * projection;
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

    const QPointF adjusted = applyRulerSnap(m_currentStrokePath.currentPosition(), scenePos);

    if (adjusted == m_currentStrokePath.currentPosition())
    {
        return;
    }

    m_currentStrokePath.lineTo(adjusted);
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

    QGraphicsView::keyPressEvent(event);
}

void Carta::keyReleaseEvent(QKeyEvent *event)
{
    if (!event)
    {
        QGraphicsView::keyReleaseEvent(event);
        return;
    }

    QGraphicsView::keyReleaseEvent(event);
}

void Carta::focusInEvent(QFocusEvent *event)
{
    QGraphicsView::focusInEvent(event);
}

void Carta::focusOutEvent(QFocusEvent *event)
{
    QGraphicsView::focusOutEvent(event);
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

void Carta::setProjectionLinesVisible(bool visible)
{
    m_showProjectionLines = visible;
    if (visible)
    {
        updateProjectionLines();
    }
    else
    {
        clearProjectionLines();
    }
}

void Carta::updateProjectionLines()
{
    clearProjectionLines();

    if (!m_showProjectionLines)
    {
        return;
    }

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
