#ifndef CARTA_H
#define CARTA_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPoint>

class QString;
class QWheelEvent;
class QMouseEvent;
class QResizeEvent;
class QPixmap;
class QGraphicsPixmapItem;

class Carta : public QGraphicsView
{
    Q_OBJECT

public:
    explicit Carta(QWidget *parent = nullptr);

    bool loadMap(const QString &filePath);
    bool setMapPixmap(const QPixmap &pixmap);
    void clearMap();
    void setZoomRange(qreal minFactor, qreal maxFactor);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

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

    void applyScale(qreal factor);
    void anchorMapToSide();
    void fitMapToViewportHeight();
    qreal minAllowedScale() const;
    qreal maxAllowedScale() const;
};

#endif // CARTA_H
