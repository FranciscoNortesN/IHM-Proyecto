#include "mapoverlaypanel.h"

#include <QApplication>
#include <QButtonGroup>
#include <QColorDialog>
#include <QDrag>
#include <QEvent>
#include <QFrame>
#include <QFontMetrics>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QSlider>
#include <QSpacerItem>
#include <QStyle>
#include <QSvgRenderer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>

#include <QSizePolicy>
#include <algorithm>

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

    QPixmap colorizePixmap(const QPixmap &source, const QColor &color)
    {
        if (source.isNull())
        {
            return source;
        }

        QImage image = source.toImage().convertToFormat(QImage::Format_ARGB32);
        for (int y = 0; y < image.height(); ++y)
        {
            QRgb *row = reinterpret_cast<QRgb *>(image.scanLine(y));
            for (int x = 0; x < image.width(); ++x)
            {
                const int alpha = qAlpha(row[x]);
                row[x] = qRgba(color.red(), color.green(), color.blue(), alpha);
            }
        }

        return QPixmap::fromImage(image);
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
        const QPixmap baseIcon = renderSvgPixmap(descriptor.resourcePath, kToolIconPrimarySize);
        m_dragPixmap = renderSvgPixmap(descriptor.resourcePath, kToolDragIconSize);
        m_iconPixmap = colorizePixmap(baseIcon, Qt::white);
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
    buildUi();
}

void MapOverlayPanel::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateTitleElision();
}

void MapOverlayPanel::buildUi()
{
    auto *panelLayout = new QVBoxLayout(this);
    panelLayout->setContentsMargins(0, 0, 0, 0);
    panelLayout->setSpacing(0); // remove gap between header/tool area

    m_headerRow = new QWidget(this);
    m_headerRow->setObjectName(QStringLiteral("overlayHeader"));
    auto *headerLayout = new QHBoxLayout(m_headerRow);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(0); // title + folder touch each other

    QWidget *titleCard = new QWidget(m_headerRow);
    titleCard->setObjectName(QStringLiteral("overlayTitleCard"));
    titleCard->setMinimumHeight(50);
    titleCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *titleLayout = new QVBoxLayout(titleCard);
    titleLayout->setContentsMargins(12, 12, 12, 12);
    titleLayout->setSpacing(0);

    m_titleLabel = new QLabel(tr("Estrecho de Gibraltar"), titleCard);
    m_titleLabel->setObjectName(QStringLiteral("overlayTitleLabel"));
    m_titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    titleLayout->addWidget(m_titleLabel);
    m_titleText = m_titleLabel->text();
    m_titleLabel->setToolTip(m_titleText);

    m_changeButton = new QToolButton(m_headerRow);
    m_changeButton->setObjectName(QStringLiteral("overlayChangeButton"));
    m_changeButton->setFixedSize(50, 50);
    m_changeButton->setIcon(QIcon(QStringLiteral(":/assets/icons/folder.svg")));
    m_changeButton->setIconSize(QSize(28, 28));
    m_changeButton->setToolTip(tr("Cambiar mapa (Ctrl+O)"));
    m_changeButton->setCursor(Qt::PointingHandCursor);
    connect(m_changeButton, &QToolButton::clicked, this, &MapOverlayPanel::changeMapRequested);

    headerLayout->addWidget(titleCard, 0);
    headerLayout->addWidget(m_changeButton, 0, Qt::AlignTop);
    headerLayout->addStretch(1);

    panelLayout->addWidget(m_headerRow, 0);

    m_toolsCard = new QWidget(this);
    m_toolsCard->setObjectName(QStringLiteral("overlayToolsCard"));
    m_toolsLayout = new QVBoxLayout(m_toolsCard);
    m_toolsLayout->setContentsMargins(1, 1, 1, 1);
    m_toolsLayout->setSpacing(0);

    m_toolPane = new QWidget(m_toolsCard);
    m_toolPane->setObjectName(QStringLiteral("overlayToolPane"));
    m_toolPane->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_toolPaneLayout = new QHBoxLayout(m_toolPane);
    m_toolPaneLayout->setContentsMargins(12, 12, 12, 12);
    m_toolPaneLayout->setSpacing(16);
    m_toolPaneLayout->setAlignment(Qt::AlignLeft);
    m_toolsLayout->addWidget(m_toolPane);

    m_toolButtonsContainer = new QWidget(m_toolPane);
    m_toolButtonsContainer->setObjectName(QStringLiteral("overlayToolStrip"));
    m_toolButtonsLayout = new QHBoxLayout(m_toolButtonsContainer);
    m_toolButtonsLayout->setContentsMargins(0, 0, 0, 0);
    m_toolButtonsLayout->setSpacing(8);
    m_toolPaneLayout->addWidget(m_toolButtonsContainer, 0, Qt::AlignLeft);

    m_actionGridWidget = new QWidget(m_toolPane);
    m_actionGridWidget->setObjectName(QStringLiteral("overlayActionGrid"));
    m_actionGridWidget->setMaximumWidth(360);
    m_actionGridLayout = new QGridLayout(m_actionGridWidget);
    m_actionGridLayout->setContentsMargins(0, 0, 0, 0);
    m_actionGridLayout->setHorizontalSpacing(8);
    m_actionGridLayout->setVerticalSpacing(8);
    m_toolPaneLayout->addWidget(m_actionGridWidget, 1);

    m_toolPaneSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_toolPaneLayout->addItem(m_toolPaneSpacer);

    m_toolsLayout->addStretch(1);
    panelLayout->addWidget(m_toolsCard, 1);

    buildActionGrid();

    registerDragHandle(m_headerRow);
    registerDragHandle(titleCard);
}

void MapOverlayPanel::buildActionGrid()
{
    if (!m_actionGridLayout)
    {
        return;
    }

    m_modeButtonGroup = new QButtonGroup(this);
    m_modeButtonGroup->setExclusive(true);

    auto createModeButton = [this](const QString &objectName, const QString &toolTip, const QIcon &icon, Mode mode)
    {
        QToolButton *button = makeActionButton(objectName, icon, toolTip, true);
        button->setCheckable(true);
        m_modeButtonGroup->addButton(button, modeToId(mode));
        return button;
    };

    m_dragModeButton = createModeButton(QStringLiteral("actionDragMode"), tr("Modo desplazamiento (D)"),
                                        QIcon(QStringLiteral(":/assets/icons/drag.svg")), Mode::Drag);
    m_paintModeButton = createModeButton(QStringLiteral("actionPaintMode"), tr("Modo dibujo (P)"),
                                         QIcon(QStringLiteral(":/assets/icons/paint.svg")), Mode::Paint);
    m_eraseModeButton = createModeButton(QStringLiteral("actionEraseMode"), tr("Modo borrador (E)"),
                                         QIcon(QStringLiteral(":/assets/icons/erase.svg")), Mode::Erase);
    m_pointModeButton = createModeButton(QStringLiteral("actionPointMode"), tr("Marcar puntos (O)"),
                                         QIcon(QStringLiteral(":/assets/icons/point.svg")), Mode::Point);
    m_textModeButton = createModeButton(QStringLiteral("actionTextMode"), tr("Añadir texto (T)"),
                                        QIcon(QStringLiteral(":/assets/icons/text.svg")), Mode::Text);
    m_undoButton = makeActionButton(QStringLiteral("actionUndo"),
                                    QIcon(QStringLiteral(":/assets/icons/undo.svg")), tr("Deshacer (Ctrl+Z)"));
    m_gridButton = makeActionButton(QStringLiteral("actionGrid"),
                                    QIcon(QStringLiteral(":/assets/icons/grid.svg")), tr("Mostrar proyecciones"), true);
    m_settingsButton = makeActionButton(QStringLiteral("actionSettings"),
                                        QIcon(QStringLiteral(":/assets/icons/settings.svg")), tr("Ajustes"));
    m_colorButton = makeActionButton(QStringLiteral("actionColor"),
                                     QIcon(QStringLiteral(":/assets/icons/palette.svg")), tr("Color actual"));
    m_clearButton = makeActionButton(QStringLiteral("actionClear"),
                                     QIcon(QStringLiteral(":/assets/icons/trash.svg")), tr("Eliminar ediciones (Supr)"));

    createSettingsMenu();
    m_settingsButton->setMenu(m_settingsMenu);
    m_settingsButton->setPopupMode(QToolButton::InstantPopup);

    connect(m_modeButtonGroup, &QButtonGroup::idClicked, this, &MapOverlayPanel::handleModeButtonClicked);
    connect(m_undoButton, &QToolButton::clicked, this, &MapOverlayPanel::undoRequested);
    connect(m_gridButton, &QToolButton::toggled, this, &MapOverlayPanel::gridToggled);
    connect(m_clearButton, &QToolButton::clicked, this, &MapOverlayPanel::clearEditsRequested);
    connect(m_colorButton, &QToolButton::clicked, this, [this]()
            {
        const QColor chosen = QColorDialog::getColor(m_currentColor, this, tr("Seleccionar color"),
                                                     QColorDialog::ShowAlphaChannel);
        if (chosen.isValid())
        {
            setCurrentColor(chosen);
            emit colorPicked(chosen);
        } });

    updateColorButtonStyle();

    auto placeButton = [this](QToolButton *button, int row, int column)
    {
        if (!m_actionGridLayout)
        {
            return;
        }
        m_actionGridLayout->addWidget(button, row, column);
    };

    placeButton(m_dragModeButton, 0, 0);
    placeButton(m_paintModeButton, 0, 1);
    placeButton(m_eraseModeButton, 0, 2);
    placeButton(m_textModeButton, 0, 3);
    placeButton(m_pointModeButton, 0, 4);

    placeButton(m_undoButton, 1, 0);
    placeButton(m_gridButton, 1, 1);
    placeButton(m_colorButton, 1, 2);
    placeButton(m_settingsButton, 1, 3);
    placeButton(m_clearButton, 1, 4);

    setActiveMode(Mode::Drag);
}

void MapOverlayPanel::createSettingsMenu()
{
    if (m_settingsMenu)
    {
        return;
    }

    m_settingsMenu = new QMenu(this);
    auto *container = new QWidget(m_settingsMenu);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    auto *thicknessLabel = new QLabel(tr("Grosor"), container);
    m_thicknessSlider = new QSlider(Qt::Horizontal, container);
    m_thicknessSlider->setRange(2, 24);
    m_thicknessSlider->setValue(8);

    auto *opacityLabel = new QLabel(tr("Opacidad"), container);
    m_opacitySlider = new QSlider(Qt::Horizontal, container);
    m_opacitySlider->setRange(10, 100);
    m_opacitySlider->setValue(85);

    layout->addWidget(thicknessLabel);
    layout->addWidget(m_thicknessSlider);
    layout->addWidget(opacityLabel);
    layout->addWidget(m_opacitySlider);

    auto *action = new QWidgetAction(m_settingsMenu);
    action->setDefaultWidget(container);
    m_settingsMenu->addAction(action);

    connect(m_thicknessSlider, &QSlider::valueChanged, this, [this](int value)
            {
        if (m_updatingSettingsUi)
        {
            return;
        }
        emit strokeWidthChanged(value); });

    connect(m_opacitySlider, &QSlider::valueChanged, this, [this](int value)
            {
        if (m_updatingSettingsUi)
        {
            return;
        }
        emit strokeOpacityChanged(value); });
}

QToolButton *MapOverlayPanel::makeActionButton(const QString &objectName, const QIcon &icon,
                                               const QString &toolTip, bool checkable)
{
    auto *button = new QToolButton(m_actionGridWidget);
    button->setObjectName(objectName);
    button->setToolTip(toolTip);
    button->setIcon(icon);
    button->setCheckable(checkable);
    button->setAutoRaise(false);
    button->setFixedSize(56, 56);
    button->setIconSize(QSize(28, 28));
    button->setCursor(Qt::PointingHandCursor);
    return button;
}

void MapOverlayPanel::setTitle(const QString &title)
{
    m_titleText = title;
    if (m_titleLabel)
    {
        m_titleLabel->setToolTip(title);
        updateTitleElision();
    }
}

QString MapOverlayPanel::title() const
{
    return m_titleText;
}

void MapOverlayPanel::updateTitleElision()
{
    if (!m_titleLabel)
    {
        return;
    }

    if (m_titleText.isEmpty())
    {
        m_titleLabel->clear();
        return;
    }

    const int availableWidth = m_titleLabel->width();
    if (availableWidth <= 0)
    {
        m_titleLabel->setText(m_titleText);
        return;
    }

    const QFontMetrics metrics(m_titleLabel->font());
    const QString elided = metrics.elidedText(m_titleText, Qt::ElideRight, availableWidth);
    m_titleLabel->setText(elided);
}

int MapOverlayPanel::minimumVisibleHeight() const
{
    constexpr int kFallbackHeight = 48;
    if (!m_headerRow)
    {
        return kFallbackHeight;
    }

    const int measured = m_headerRow->isVisible() ? m_headerRow->height()
                                                  : m_headerRow->sizeHint().height();
    return std::max(measured, kFallbackHeight);
}

void MapOverlayPanel::setToolDescriptors(const QList<MapToolDescriptor> &descriptors)
{
    m_toolDescriptors = descriptors;
    rebuildToolPane();
}

void MapOverlayPanel::setActiveMode(Mode mode)
{
    if (m_activeMode == mode)
    {
        return;
    }

    m_activeMode = mode;
    if (m_modeButtonGroup)
    {
        if (QAbstractButton *button = m_modeButtonGroup->button(modeToId(mode)))
        {
            button->setChecked(true);
        }
    }

    switch (mode)
    {
    case Mode::Drag:
        emit dragModeSelected();
        break;
    case Mode::Paint:
        emit paintModeSelected();
        break;
    case Mode::Erase:
        emit eraseModeSelected();
        break;
    case Mode::Text:
        emit textModeSelected();
        break;
    case Mode::Point:
        emit pointModeSelected();
        break;
    case Mode::Line:
        // Line mode removed; no action
        break;
    }
}

void MapOverlayPanel::setCurrentColor(const QColor &color)
{
    if (!color.isValid())
    {
        return;
    }
    m_currentColor = color;
    updateColorButtonStyle();
}

void MapOverlayPanel::setPaintSettings(int thickness, int opacityPercent)
{
    m_updatingSettingsUi = true;
    if (m_thicknessSlider)
    {
        m_thicknessSlider->setValue(thickness);
    }
    if (m_opacitySlider)
    {
        m_opacitySlider->setValue(opacityPercent);
    }
    m_updatingSettingsUi = false;
}

void MapOverlayPanel::rebuildToolPane()
{
    if (!m_toolButtonsLayout)
    {
        return;
    }

    destroyToolButtons();

    for (const MapToolDescriptor &descriptor : m_toolDescriptors)
    {
        auto *button = new ToolPaletteButton(descriptor, m_toolButtonsContainer);
        m_toolButtons.append(button);
        m_toolButtonsLayout->addWidget(button);
    }
}

void MapOverlayPanel::destroyToolButtons()
{
    for (ToolPaletteButton *button : m_toolButtons)
    {
        if (m_toolButtonsLayout)
        {
            m_toolButtonsLayout->removeWidget(button);
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

void MapOverlayPanel::handleModeButtonClicked(int id)
{
    // If user selects another tool, turn off grid/crosshair mode
    if (m_gridButton && m_gridButton->isChecked())
    {
        m_gridButton->blockSignals(true);
        m_gridButton->setChecked(false);
        m_gridButton->blockSignals(false);
        emit gridToggled(false);
    }
    setActiveMode(idToMode(id));
}

int MapOverlayPanel::modeToId(Mode mode)
{
    return static_cast<int>(mode);
}

MapOverlayPanel::Mode MapOverlayPanel::idToMode(int id)
{
    switch (static_cast<Mode>(id))
    {
    case Mode::Paint:
        return Mode::Paint;
    case Mode::Erase:
        return Mode::Erase;
    case Mode::Text:
        return Mode::Text;
    case Mode::Point:
        return Mode::Point;
    case Mode::Drag:
    default:
        return Mode::Drag;
    }
}

void MapOverlayPanel::updateColorButtonStyle()
{
    if (!m_colorButton)
        return;

    // Render the palette SVG and tint it with the selected color for the icon only.
    const int iconSize = m_colorButton->iconSize().width();
    const QPixmap base = renderSvgPixmap(QStringLiteral(":/assets/icons/palette.svg"), iconSize);
    const QPixmap colored = base.isNull() ? QPixmap() : colorizePixmap(base, m_currentColor);

    if (!colored.isNull()) {
        m_colorButton->setIcon(QIcon(colored));
    } else {
        m_colorButton->setIcon(QIcon(QStringLiteral(":/assets/icons/palette.svg")));
    }
    // Remove any custom stylesheet so it uses the default button style
    m_colorButton->setStyleSheet("");
}

QPoint MapOverlayPanel::mouseGlobalPos(const QMouseEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->globalPosition().toPoint();
#else
    return event->globalPos();
#endif
}
