#include "toastnotification.h"
#include <QPainter>
#include <QFontMetrics>
#include <QApplication>
#include <QGraphicsOpacityEffect>

ToastNotification::ToastNotification(QWidget *parent)
    : QWidget(parent)
    , m_currentType(Info)
{
    // No usar window flags, será un widget hijo normal
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAutoFillBackground(false);
    
    m_label = new QLabel(this);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setWordWrap(true);
    m_label->setStyleSheet("color: white; padding: 16px 24px;");
    
    QFont font = m_label->font();
    font.setPointSize(12);
    font.setBold(true);
    m_label->setFont(font);
    
    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    connect(m_hideTimer, &QTimer::timeout, this, [this]() {
        // Fade out animation
        auto *effect = new QGraphicsOpacityEffect(this);
        setGraphicsEffect(effect);
        
        m_fadeAnimation = new QPropertyAnimation(effect, "opacity", this);
        m_fadeAnimation->setDuration(300);
        m_fadeAnimation->setStartValue(1.0);
        m_fadeAnimation->setEndValue(0.0);
        
        connect(m_fadeAnimation, &QPropertyAnimation::finished, this, &QWidget::hide);
        m_fadeAnimation->start(QPropertyAnimation::DeleteWhenStopped);
    });
    
    hide();
}

void ToastNotification::showMessage(const QString &message, NotificationType type, int durationMs)
{
    m_currentType = type;
    m_label->setText(message);
    
    // Reset opacity if there was a previous fade
    if (graphicsEffect()) {
        setGraphicsEffect(nullptr);
    }
    
    adjustSize();
    
    show();
    raise();
    
    // Position at bottom right of parent after showing
    if (parentWidget()) {
        int finalX = parentWidget()->width() - width() - 20;
        int finalY = parentWidget()->height() - height() - 40;
        
        // Start from outside the screen (right side)
        move(parentWidget()->width(), finalY);
        
        // Animate slide in from right
        QPropertyAnimation *slideAnimation = new QPropertyAnimation(this, "pos", this);
        slideAnimation->setDuration(400);
        slideAnimation->setStartValue(QPoint(parentWidget()->width(), finalY));
        slideAnimation->setEndValue(QPoint(finalX, finalY));
        slideAnimation->setEasingCurve(QEasingCurve::OutCubic);
        slideAnimation->start(QPropertyAnimation::DeleteWhenStopped);
    }
    
    m_hideTimer->start(durationMs);
}

void ToastNotification::adjustSize()
{
    QFontMetrics fm(m_label->font());
    int maxWidth = 600;
    if (parentWidget()) {
        maxWidth = qMin(600, parentWidget()->width() - 80);
    }
    
    QRect boundingRect = fm.boundingRect(0, 0, maxWidth, 1000, 
                                         Qt::AlignCenter | Qt::TextWordWrap, 
                                         m_label->text());
    
    int width = boundingRect.width() + 48;
    int height = boundingRect.height() + 32;
    
    setFixedSize(width, height);
    m_label->setGeometry(0, 0, width, height);
}

void ToastNotification::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw shadow
    QRect shadowRect = rect().adjusted(2, 2, 2, 2);
    painter.setBrush(QColor(0, 0, 0, 80));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(shadowRect, 8, 8);
    
    // Draw rounded rectangle background
    painter.setBrush(getBackgroundColor());
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), 8, 8);
}

QColor ToastNotification::getBackgroundColor() const
{
    switch (m_currentType) {
        case Success:
            return QColor(13, 30, 46, 240);
        case Warning:
            return QColor(237, 108, 2, 240); // Orange
        case Error:
            return QColor(211, 47, 47, 240); // Red
        case Info:
        default:
            return QColor(33, 150, 243, 240); // Blue
    }
}

QColor ToastNotification::getTextColor() const
{
    return QColor(255, 255, 255);
}
