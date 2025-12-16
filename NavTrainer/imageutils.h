#ifndef IMAGEUTILS_H
#define IMAGEUTILS_H

#include <QPixmap>
#include <QImage>

class ImageUtils
{
public:
    // Convierte un QPixmap en uno circular
    static QPixmap makeCircular(const QPixmap &source, int size);
    
    // Convierte un QImage en QPixmap circular
    static QPixmap makeCircularFromImage(const QImage &image, int size);
    
    // Recorta la imagen a un cuadrado centrado antes de hacerla circular
    static QPixmap makeCircularCropped(const QPixmap &source, int size);
};

#endif // IMAGEUTILS_H
