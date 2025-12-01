#include "mapoverlaypanel.h"

#include <QApplication>
#include <QDrag>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QSpacerItem>
#include <QStyle>
#include <QSvgRenderer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QSizePolicy>

namespace
{
constexpr int kToolIconPrimarySize = 64;
constexpr int kToolDragIconSize = 96;

QPixmap renderSvgPixmap(const QString &resourcePath, int size)
{
    QSvgRenderer renderer(resourcePath);
    if (!renderer.isValid())
    {
        return QPixmap();
    }

    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    renderer.render(&painter);
    return pix;
}
} // namespace

class ToolPaletteButton : public QFrame
{
public:
    explicit ToolPaletteButton(const MapToolDescriptor &descriptor, QWidget *parent = nullptr)
        : QFrame(parent), m_descriptor(descriptor)
    {
        setObjectName(QStringLiteral("overlayToolButton"));
        setCursor(Qt::OpenHandCursor);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        setFixedSize(88, 88);
        setAttribute(Qt::WA_StyledBackground, true);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(4);

        m_iconLabel = new QLabel(this);
        m_iconLabel->setAlignment(Qt::AlignCenter);
        m_iconLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        layout->addWidget(m_iconLabel, 1);

        setToolTip(descriptor.title);
        setDescriptor(descriptor);
    }

    void setDescriptor(const MapToolDescriptor &descriptor)
    {
        m_descriptor = descriptor;
        m_iconPixmap = renderSvgPixmap(descriptor.resourcePath, kToolIconPrimarySize);
        m_dragPixmap = renderSvgPixmap(descriptor.resourcePath, kToolDragIconSize);
        if (!m_iconPixmap.isNull())
        {
            m_iconLabel->setPixmap(m_iconPixmap);
        }
    }

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
        {
            m_dragStartPos = event->pos();
        }
        QFrame::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (!(event->buttons() & Qt::LeftButton))
        {
            QFrame::mouseMoveEvent(event);
            return;
        }

        if ((event->pos() - m_dragStartPos).manhattanLength() < QApplication::startDragDistance())
        {
            QFrame::mouseMoveEvent(event);
            return;
        }

        startDrag();
    }

private:
    MapToolDescriptor m_descriptor;
    QLabel *m_iconLabel = nullptr;
    QPoint m_dragStartPos;
    QPixmap m_iconPixmap;
    QPixmap m_dragPixmap;

    void startDrag()
    {
        if (m_descriptor.id.isEmpty())
        {
            return;
        }

        auto *drag = new QDrag(this);
        auto *mime = new QMimeData();
        mime->setData(MapToolMime::ToolId, m_descriptor.id.toUtf8());
        mime->setData(MapToolMime::ToolSource, m_descriptor.resourcePath.toUtf8());
        drag->setMimeData(mime);

        if (!m_dragPixmap.isNull())
        {
            drag->setPixmap(m_dragPixmap);
            drag->setHotSpot(QPoint(m_dragPixmap.width() / 2, m_dragPixmap.height() / 2));
        }

        drag->exec(Qt::CopyAction);
    }
};

MapOverlayPanel::MapOverlayPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("mapOverlayPanel"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedSize(260, 260);
    buildUi();
}

void MapOverlayPanel::buildUi()
{
    auto *panelLayout = new QVBoxLayout(this);
    panelLayout->setContentsMargins(12, 12, 12, 12);
    panelLayout->setSpacing(12);

    auto *headerRow = new QWidget(this);
    headerRow->setObjectName(QStringLiteral("overlayHeader"));
    auto *headerLayout = new QHBoxLayout(headerRow);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(12);

    QWidget *titleCard = new QWidget(headerRow);
    titleCard->setObjectName(QStringLiteral("overlayTitleCard"));
    titleCard->setMinimumHeight(72);
    titleCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *titleLayout = new QVBoxLayout(titleCard);
    titleLayout->setContentsMargins(12, 10, 12, 10);
    titleLayout->setSpacing(4);

    m_titleLabel = new QLabel(tr("Carta náutica"), titleCard);
    m_titleLabel->setObjectName(QStringLiteral("overlayTitleLabel"));
    m_titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_titleLabel->setWordWrap(true);
    titleLayout->addWidget(m_titleLabel);

    m_changeButton = new QToolButton(headerRow);
    m_changeButton->setObjectName(QStringLiteral("overlayChangeButton"));
    m_changeButton->setFixedSize(72, 72);
    m_changeButton->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    m_changeButton->setIconSize(QSize(28, 28));
    m_changeButton->setCursor(Qt::PointingHandCursor);
    connect(m_changeButton, &QToolButton::clicked, this, &MapOverlayPanel::changeMapRequested);

    headerLayout->addWidget(titleCard, 1);
    headerLayout->addWidget(m_changeButton, 0, Qt::AlignTop);
    panelLayout->addWidget(headerRow, 0);

    m_toolsCard = new QWidget(this);
    m_toolsCard->setObjectName(QStringLiteral("overlayToolsCard"));
    m_toolsLayout = new QVBoxLayout(m_toolsCard);
    m_toolsLayout->setContentsMargins(12, 12, 12, 12);
    m_toolsLayout->setSpacing(8);

    m_toolPane = new QWidget(m_toolsCard);
    m_toolPane->setObjectName(QStringLiteral("overlayToolPane"));
    m_toolPane->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_toolPaneLayout = new QHBoxLayout(m_toolPane);
    m_toolPaneLayout->setContentsMargins(0, 0, 0, 0);
    m_toolPaneLayout->setSpacing(12);
    m_toolsLayout->addWidget(m_toolPane, 0);

    m_toolPaneSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_toolPaneLayout->addItem(m_toolPaneSpacer);

    auto *spacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    m_toolsLayout->addItem(spacer);
    panelLayout->addWidget(m_toolsCard, 1);

    registerDragHandle(headerRow);
    registerDragHandle(titleCard);
}

void MapOverlayPanel::setTitle(const QString &title)
{
    if (m_titleLabel)
    {
        m_titleLabel->setText(title);
    }
}

QString MapOverlayPanel::title() const
{
    return m_titleLabel ? m_titleLabel->text() : QString();
}

void MapOverlayPanel::setToolDescriptors(const QList<MapToolDescriptor> &descriptors)
{
    m_toolDescriptors = descriptors;
    rebuildToolPane();
}

void MapOverlayPanel::rebuildToolPane()
{
    if (!m_toolPaneLayout)
    {
        return;
    }

    if (m_toolPaneSpacer)
    {
        m_toolPaneLayout->removeItem(m_toolPaneSpacer);
    }

    destroyToolButtons();

    for (const MapToolDescriptor &descriptor : m_toolDescriptors)
    {
        auto *button = new ToolPaletteButton(descriptor, m_toolPane);
        m_toolButtons.append(button);
        m_toolPaneLayout->addWidget(button);
    }

    if (m_toolPaneSpacer)
    {
        m_toolPaneLayout->addItem(m_toolPaneSpacer);
    }
}

void MapOverlayPanel::destroyToolButtons()
{
    for (ToolPaletteButton *button : m_toolButtons)
    {
        if (m_toolPaneLayout)
        {
            m_toolPaneLayout->removeWidget(button);
        }
        button->deleteLater();
    }
    m_toolButtons.clear();
}

void MapOverlayPanel::registerDragHandle(QWidget *widget)
{
    if (!widget)
    {
        return;
    }

    widget->installEventFilter(this);
    m_dragHandles.append(widget);
}

bool MapOverlayPanel::eventFilter(QObject *watched, QEvent *event)
{
    QWidget *widget = qobject_cast<QWidget *>(watched);
    if (!widget || !m_dragHandles.contains(widget))
    {
        return QWidget::eventFilter(watched, event);
    }

    if (event->type() == QEvent::MouseButtonPress ||
        event->type() == QEvent::MouseButtonRelease ||
        event->type() == QEvent::MouseMove)
    {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        handleDragEvent(mouseEvent);
        return true;
    }

    return QWidget::eventFilter(watched, event);
}

void MapOverlayPanel::mousePressEvent(QMouseEvent *event)
{
    handleDragEvent(event);
    QWidget::mousePressEvent(event);
}

void MapOverlayPanel::mouseMoveEvent(QMouseEvent *event)
{
    handleDragEvent(event);
    QWidget::mouseMoveEvent(event);
}

void MapOverlayPanel::mouseReleaseEvent(QMouseEvent *event)
{
    handleDragEvent(event);
    QWidget::mouseReleaseEvent(event);
}

void MapOverlayPanel::handleDragEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && event->type() == QEvent::MouseButtonPress)
    {
        m_dragging = true;
        m_lastGlobalPos = mouseGlobalPos(event);
        event->accept();
        return;
    }

    if (event->type() == QEvent::MouseButtonRelease)
    {
        if (event->button() == Qt::LeftButton)
        {
            m_dragging = false;
        }
        return;
    }

    if (event->type() == QEvent::MouseMove && m_dragging)
    {
        const QPoint currentGlobal = mouseGlobalPos(event);
        const QPoint delta = currentGlobal - m_lastGlobalPos;
        m_lastGlobalPos = currentGlobal;
        if (!delta.isNull())
        {
            emit dragDeltaRequested(delta);
        }
        event->accept();
    }
}

QPoint MapOverlayPanel::mouseGlobalPos(const QMouseEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->globalPosition().toPoint();
#else
    return event->globalPos();
#endif
}
