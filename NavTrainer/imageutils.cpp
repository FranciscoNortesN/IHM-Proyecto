#include "imageutils.h"
#include <QPainter>
#include <QPainterPath>

QPixmap ImageUtils::makeCircular(const QPixmap &source, int size)
{
    if (source.isNull()) {
        return QPixmap();
    }
    
    // Escalar el pixmap al tamaño deseado
    QPixmap scaledPixmap = source.scaled(
        size, size,
        Qt::KeepAspectRatioByExpanding,
        Qt::SmoothTransformation
    );
    
    // Crear pixmap circular
    QPixmap roundedPixmap(size, size);
    roundedPixmap.fill(Qt::transparent);
    
    QPainter painter(&roundedPixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    // Crear máscara circular
    QPainterPath path;
    path.addEllipse(0, 0, size, size);
    painter.setClipPath(path);
    
    // Centrar la imagen si es necesaria
    int x = (size - scaledPixmap.width()) / 2;
    int y = (size - scaledPixmap.height()) / 2;
    painter.drawPixmap(x, y, scaledPixmap);
    painter.end();
    
    return roundedPixmap;
}

QPixmap ImageUtils::makeCircularFromImage(const QImage &image, int size)
{
    if (image.isNull()) {
        return QPixmap();
    }
    
    return makeCircular(QPixmap::fromImage(image), size);
}

QPixmap ImageUtils::makeCircularCropped(const QPixmap &source, int size)
{
    if (source.isNull()) {
        return QPixmap();
    }
    
    // Primero recortar a un cuadrado centrado
    int minSize = qMin(source.width(), source.height());
    int x = (source.width() - minSize) / 2;
    int y = (source.height() - minSize) / 2;
    QPixmap squarePixmap = source.copy(x, y, minSize, minSize);
    
    // Luego hacer circular
    return makeCircular(squarePixmap, size);
}
