#ifndef TOASTNOTIFICATION_H
#define TOASTNOTIFICATION_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QPropertyAnimation>

class ToastNotification : public QWidget
{
    Q_OBJECT
    
public:
    enum NotificationType {
        Info,
        Success,
        Warning,
        Error
    };
    
    explicit ToastNotification(QWidget *parent = nullptr);
    
    void showMessage(const QString &message, NotificationType type = Info, int durationMs = 5000);
    
protected:
    void paintEvent(QPaintEvent *event) override;
    
private:
    QLabel *m_label;
    QTimer *m_hideTimer;
    QPropertyAnimation *m_fadeAnimation;
    NotificationType m_currentType;
    
    QColor getBackgroundColor() const;
    QColor getTextColor() const;
    void adjustSize();
};

#endif // TOASTNOTIFICATION_H
