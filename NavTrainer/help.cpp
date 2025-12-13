#include "help.h"
#include "ui_help.h"
#include <QFile>
#include <QTextStream>
#include <QScrollBar>
#include <QDebug>
#include <QRegularExpression>

Help::Help(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Help)
    , currentResultIndex(-1)
{
    ui->setupUi(this);
    
    // Ocultar el banner por defecto
    ui->banner->setVisible(false);
    
    // Configurar placeholder del campo de búsqueda
    ui->search_field->setPlaceholderText("Introduce aquí el término que quieres buscar");
    ui->search_field->clear();
    
    // Conectar señales y slots
    connect(ui->search_button, &QPushButton::clicked, this, &Help::onSearchButtonClicked);
    connect(ui->search_field, &QLineEdit::returnPressed, this, &Help::onSearchButtonClicked);
    connect(ui->next_button, &QPushButton::clicked, this, &Help::onNextButtonClicked);
    connect(ui->prev_button, &QPushButton::clicked, this, &Help::onPrevButtonClicked);
    connect(ui->close_button, &QPushButton::clicked, this, &Help::onCloseBannerClicked);
    
    // Conectar los TreeWidgets
    connect(ui->intro_index, &QTreeWidget::itemClicked, this, &Help::onIndexItemClicked);
    connect(ui->registro_index, &QTreeWidget::itemClicked, this, &Help::onIndexItemClicked);
    connect(ui->map_index, &QTreeWidget::itemClicked, this, &Help::onIndexItemClicked);
    connect(ui->prob_index, &QTreeWidget::itemClicked, this, &Help::onIndexItemClicked);
    connect(ui->stats_index, &QTreeWidget::itemClicked, this, &Help::onIndexItemClicked);
    
    // Configurar los TreeWidgets
    ui->intro_index->setHeaderHidden(true);
    ui->registro_index->setHeaderHidden(true);
    ui->map_index->setHeaderHidden(true);
    ui->prob_index->setHeaderHidden(true);
    ui->stats_index->setHeaderHidden(true);
    
    // Cargar contenido de los archivos markdown
    loadMarkdownContent();
    
    // Configurar índices
    setupIndex();
}

Help::~Help()
{
    delete ui;
}

void Help::loadMarkdownContent()
{
    loadMarkdownFile(":/assets/help/introduccion.md", ui->intro_md);
    loadMarkdownFile(":/assets/help/registro.md", ui->registro_md);
    loadMarkdownFile(":/assets/help/mapa.md", ui->map_md);
    loadMarkdownFile(":/assets/help/problemas.md", ui->prob_md);
    loadMarkdownFile(":/assets/help/estadisticas.md", ui->stats_md);
}

void Help::loadMarkdownFile(const QString &resourcePath, QTextBrowser *browser)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "No se pudo abrir el archivo:" << resourcePath;
        browser->setHtml("<p>Error al cargar el contenido de ayuda.</p>");
        return;
    }
    
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString markdown = in.readAll();
    file.close();
    
    // Convertir Markdown a HTML
    QString html = markdownToHtml(markdown);
    browser->setHtml(html);
    
    // Guardar el contenido original para búsqueda
    browser->setProperty("originalMarkdown", markdown);
}

QString Help::markdownToHtml(const QString &markdown)
{
    QString html = "<html><head><style>"
                  "body { font-family: 'Noto Sans', Arial, sans-serif; line-height: 1.6; padding: 20px; }"
                  "h1 { color: #4a9eff; border-bottom: 3px solid #4a9eff; padding-bottom: 10px; margin-top: 10px; }"
                  "h2 { color: #5fadff; margin-top: 30px; border-bottom: 2px solid #5fadff; padding-bottom: 8px; }"
                  "h3 { color: #74bcff; margin-top: 20px; }"
                  "h4 { color: #89cbff; margin-top: 15px; }"
                  "code { background-color: rgba(100, 100, 100, 0.3); padding: 2px 6px; border-radius: 3px; }"
                  "pre { background-color: rgba(100, 100, 100, 0.3); padding: 15px; border-radius: 5px; overflow-x: auto; }"
                  "ul, ol { margin-left: 20px; }"
                  "li { margin-bottom: 8px; }"
                  "strong { color: #4a9eff; font-weight: bold; }"
                  ".search-highlight { background-color: #ffeb3b; color: #000000; font-weight: bold; padding: 2px 4px; border-radius: 2px; }"
                  "</style></head><body>";
    
    QString content = markdown;
    
    // Procesar títulos con anclas
    QRegularExpression h1Regex("^# (.+)$", QRegularExpression::MultilineOption);
    QRegularExpression h2Regex("^## (.+)$", QRegularExpression::MultilineOption);
    QRegularExpression h3Regex("^### (.+)$", QRegularExpression::MultilineOption);
    QRegularExpression h4Regex("^#### (.+)$", QRegularExpression::MultilineOption);
    
    content.replace(h1Regex, "<h1 id=\"\\1\">\\1</h1>");
    content.replace(h2Regex, "<h2 id=\"\\1\">\\1</h2>");
    content.replace(h3Regex, "<h3 id=\"\\1\">\\1</h3>");
    content.replace(h4Regex, "<h4 id=\"\\1\">\\1</h4>");
    
    // Procesar negritas
    QRegularExpression boldRegex("\\*\\*(.+?)\\*\\*");
    content.replace(boldRegex, "<strong>\\1</strong>");
    
    // Procesar itálicas
    QRegularExpression italicRegex("\\*(.+?)\\*");
    content.replace(italicRegex, "<em>\\1</em>");
    
    // Procesar código inline
    QRegularExpression codeRegex("`(.+?)`");
    content.replace(codeRegex, "<code>\\1</code>");
    
    // Procesar listas no ordenadas
    QRegularExpression ulItemRegex("^- (.+)$", QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator ulMatches = ulItemRegex.globalMatch(content);
    bool inList = false;
    QString result;
    int lastPos = 0;
    
    while (ulMatches.hasNext()) {
        QRegularExpressionMatch match = ulMatches.next();
        result += content.mid(lastPos, match.capturedStart() - lastPos);
        if (!inList) {
            result += "<ul>";
            inList = true;
        }
        result += "<li>" + match.captured(1) + "</li>";
        lastPos = match.capturedEnd();
    }
    result += content.mid(lastPos);
    if (inList) {
        result += "</ul>";
    }
    content = result;
    
    // Procesar listas ordenadas
    QRegularExpression olItemRegex("^\\d+\\. (.+)$", QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator olMatches = olItemRegex.globalMatch(content);
    inList = false;
    result.clear();
    lastPos = 0;
    
    while (olMatches.hasNext()) {
        QRegularExpressionMatch match = olMatches.next();
        result += content.mid(lastPos, match.capturedStart() - lastPos);
        if (!inList) {
            result += "<ol>";
            inList = true;
        }
        result += "<li>" + match.captured(1) + "</li>";
        lastPos = match.capturedEnd();
    }
    result += content.mid(lastPos);
    if (inList) {
        result += "</ol>";
    }
    content = result;
    
    // Procesar párrafos (líneas en blanco)
    content.replace("\n\n", "</p><p>");
    
    html += "<p>" + content + "</p></body></html>";
    
    return html;
}

void Help::setupIndex()
{
    parseMarkdownHeaders(ui->intro_md->property("originalMarkdown").toString(), "intro");
    parseMarkdownHeaders(ui->registro_md->property("originalMarkdown").toString(), "registro");
    parseMarkdownHeaders(ui->map_md->property("originalMarkdown").toString(), "map");
    parseMarkdownHeaders(ui->prob_md->property("originalMarkdown").toString(), "prob");
    parseMarkdownHeaders(ui->stats_md->property("originalMarkdown").toString(), "stats");
    
    populateTreeWidget(ui->intro_index, "intro");
    populateTreeWidget(ui->registro_index, "registro");
    populateTreeWidget(ui->map_index, "map");
    populateTreeWidget(ui->prob_index, "prob");
    populateTreeWidget(ui->stats_index, "stats");
}

void Help::parseMarkdownHeaders(const QString &content, const QString &tabName)
{
    QList<Section> sections;
    QStringList lines = content.split('\n');
    
    int position = 0;
    for (const QString &line : lines) {
        if (line.startsWith('#')) {
            Section section;
            section.title = line;
            section.title.remove(QRegularExpression("^#+\\s*"));
            section.anchor = section.title;
            section.position = position;
            sections.append(section);
        }
        position += line.length() + 1; // +1 por el salto de línea
    }
    
    sectionsByTab[tabName] = sections;
}

void Help::populateTreeWidget(QTreeWidget *tree, const QString &tabName)
{
    tree->clear();
    
    const QList<Section> &sections = sectionsByTab[tabName];
    QMap<int, QTreeWidgetItem*> levelItems;
    levelItems[0] = nullptr;
    
    for (const Section &section : sections) {
        // Contar el nivel basado en los # en el markdown original
        QTextBrowser *browser = getBrowserForTab(ui->tabWidget->indexOf(tree->parentWidget()));
        QString markdown = browser->property("originalMarkdown").toString();
        QStringList lines = markdown.split('\n');
        
        int level = 1;
        for (const QString &line : lines) {
            if (line.contains(section.title)) {
                // Contar cuántos # tiene
                for (QChar c : line) {
                    if (c == '#') level++;
                    else break;
                }
                level--; // Ajustar porque empezamos en 1
                break;
            }
        }
        
        QTreeWidgetItem *item = nullptr;
        if (level == 1 || levelItems[level - 1] == nullptr) {
            item = new QTreeWidgetItem(tree);
        } else {
            item = new QTreeWidgetItem(levelItems[level - 1]);
        }
        
        item->setText(0, section.title);
        item->setData(0, Qt::UserRole, section.position);
        
        levelItems[level] = item;
    }
    
    tree->expandAll();
}

void Help::onIndexItemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    
    // Obtener el browser del tab actual
    int tabIndex = ui->tabWidget->currentIndex();
    QTextBrowser *browser = getBrowserForTab(tabIndex);
    
    if (!browser) return;
    
    QString title = item->text(0);
    
    // Buscar el título en el documento
    QTextDocument *doc = browser->document();
    QTextCursor cursor(doc);
    
    // Buscar en el texto plano
    cursor = doc->find(title, cursor);
    
    if (!cursor.isNull()) {
        // Mover al inicio de la línea
        cursor.movePosition(QTextCursor::StartOfLine);
        
        // Seleccionar temporalmente para forzar el scroll
        QTextCursor scrollCursor = cursor;
        scrollCursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        browser->setTextCursor(scrollCursor);
        
        // Forzar el scroll
        browser->ensureCursorVisible();
        
        // Ajustar para posicionar en el tercio superior
        QScrollBar *vScrollBar = browser->verticalScrollBar();
        QRect cursorRect = browser->cursorRect(cursor);
        int viewportHeight = browser->viewport()->height();
        
        // Calcular la posición ideal: tercio superior de la ventana
        int idealPosition = vScrollBar->value() + cursorRect.top() - (viewportHeight / 3);
        vScrollBar->setValue(qMax(0, idealPosition));
        
        // Limpiar la selección visual
        cursor.clearSelection();
        browser->setTextCursor(cursor);
    }
}

void Help::onSearchButtonClicked()
{
    QString searchTerm = ui->search_field->text().trimmed();
    
    if (searchTerm.isEmpty() || searchTerm == "Introduce aquí el término que quieres buscar") {
        return;
    }
    
    // Si ya hay resultados y es el mismo término, navegar al siguiente
    if (!searchResults.isEmpty() && searchTerm.toLower() == lastSearchTerm.toLower()) {
        onNextButtonClicked();
        return;
    }
    
    performSearch(searchTerm);
}

void Help::performSearch(const QString &searchTerm)
{
    // Limpiar resultados anteriores
    searchResults.clear();
    currentResultIndex = -1;
    lastSearchTerm = searchTerm;
    
    // Buscar en todos los tabs
    for (int i = 0; i < ui->tabWidget->count(); i++) {
        QTextBrowser *browser = getBrowserForTab(i);
        if (!browser) continue;
        
        QTextDocument *doc = browser->document();
        QTextCursor searchCursor(doc);
        
        // Búsqueda case insensitive (flags vacío)
        QTextDocument::FindFlags flags;
        
        searchCursor = QTextCursor(doc);
        while (true) {
            searchCursor = doc->find(searchTerm, searchCursor, flags);
            
            if (searchCursor.isNull()) {
                break;
            }
            
            // Guardar resultado
            SearchResult result;
            result.tabIndex = i;
            result.cursor = searchCursor;
            
            // Obtener contexto (50 caracteres antes y después)
            int start = qMax(0, searchCursor.position() - 50);
            int end = qMin(doc->characterCount(), searchCursor.position() + searchTerm.length() + 50);
            result.context = doc->toPlainText().mid(start, end - start);
            
            searchResults.append(result);
        }
    }
    
    // Actualizar UI
    if (searchResults.isEmpty()) {
        updateBannerText("No se encontraron resultados para: " + searchTerm);
        showBanner(true);
    } else {
        updateBannerText("Mostrando resultados para: " + searchTerm + " (" + 
                        QString::number(searchResults.size()) + " encontrados)");
        showBanner(true);
        currentResultIndex = 0;
        highlightSearchResult(searchResults[0]);
    }
}

void Help::highlightSearchResult(const SearchResult &result)
{
    // Cambiar al tab correcto
    ui->tabWidget->setCurrentIndex(result.tabIndex);
    
    QTextBrowser *browser = getBrowserForTab(result.tabIndex);
    if (!browser) return;
    
    QTextDocument *doc = browser->document();
    
    // Primero resaltar todas las ocurrencias del término en el tab actual
    QTextCursor searchCursor(doc);
    QTextCharFormat softFormat;
    softFormat.setBackground(QBrush(QColor("#fff9c4")));
    softFormat.setForeground(QBrush(QColor("#000000")));
    
    QList<QTextCursor> allMatches;
    while (!searchCursor.isNull() && !searchCursor.atEnd()) {
        searchCursor = doc->find(lastSearchTerm, searchCursor);
        if (!searchCursor.isNull()) {
            allMatches.append(searchCursor);
        }
    }
    
    // Aplicar formato suave a todas las coincidencias
    for (const QTextCursor &match : allMatches) {
        QTextCursor tempCursor = match;
        tempCursor.mergeCharFormat(softFormat);
    }
    
    // Aplicar highlight más fuerte al resultado actual
    QTextCharFormat strongFormat;
    strongFormat.setBackground(QBrush(QColor("#ffeb3b")));
    strongFormat.setForeground(QBrush(QColor("#000000")));
    strongFormat.setFontWeight(QFont::Bold);
    
    QTextCursor highlightCursor = result.cursor;
    highlightCursor.mergeCharFormat(strongFormat);
    
    // Crear un cursor que seleccione el texto para forzar el scroll
    QTextCursor scrollCursor = result.cursor;
    scrollCursor.setPosition(result.cursor.anchor());
    scrollCursor.setPosition(result.cursor.position(), QTextCursor::KeepAnchor);
    
    browser->setTextCursor(scrollCursor);
    
    // Forzar el scroll al inicio del texto seleccionado
    browser->ensureCursorVisible();
    
    // Hacer un ajuste adicional para centrar mejor el resultado
    QScrollBar *vScrollBar = browser->verticalScrollBar();
    QTextCursor tempCursor = result.cursor;
    QRect cursorRect = browser->cursorRect(tempCursor);
    int viewportHeight = browser->viewport()->height();
    
    // Calcular la posición ideal: centrar el resultado en la ventana
    int idealPosition = vScrollBar->value() + cursorRect.top() - (viewportHeight / 3);
    vScrollBar->setValue(qMax(0, idealPosition));
    
    // Limpiar la selección visual pero mantener el cursor
    scrollCursor.clearSelection();
    browser->setTextCursor(scrollCursor);
}

void Help::clearHighlights()
{
    // Limpiar highlights en el tab actual solamente
    int currentTab = ui->tabWidget->currentIndex();
    QTextBrowser *browser = getBrowserForTab(currentTab);
    
    if (!browser) return;
    
    // Guardar la posición actual del scroll
    QScrollBar *scrollBar = browser->verticalScrollBar();
    int scrollPosition = scrollBar->value();
    
    // Recargar el HTML para limpiar completamente el formato
    QString markdown = browser->property("originalMarkdown").toString();
    browser->setHtml(markdownToHtml(markdown));
    
    // Restaurar la posición del scroll
    scrollBar->setValue(scrollPosition);
}

void Help::onNextButtonClicked()
{
    if (searchResults.isEmpty()) return;
    
    currentResultIndex++;
    if (currentResultIndex >= searchResults.size()) {
        currentResultIndex = 0; // Volver al principio
    }
    
    highlightSearchResult(searchResults[currentResultIndex]);
    updateBannerText("Mostrando resultados para: " + lastSearchTerm + 
                    " (" + QString::number(currentResultIndex + 1) + " de " + 
                    QString::number(searchResults.size()) + ")");
}

void Help::onPrevButtonClicked()
{
    if (searchResults.isEmpty()) return;
    
    currentResultIndex--;
    if (currentResultIndex < 0) {
        currentResultIndex = searchResults.size() - 1; // Ir al final
    }
    
    highlightSearchResult(searchResults[currentResultIndex]);
    updateBannerText("Mostrando resultados para: " + lastSearchTerm + 
                    " (" + QString::number(currentResultIndex + 1) + " de " + 
                    QString::number(searchResults.size()) + ")");
}

void Help::onCloseBannerClicked()
{
    showBanner(false);
    clearHighlights();
    searchResults.clear();
    currentResultIndex = -1;
}

void Help::showBanner(bool show)
{
    ui->banner->setVisible(show);
}

void Help::updateBannerText(const QString &text)
{
    ui->resultados->setText(text);
}

QTextBrowser* Help::getBrowserForTab(int tabIndex)
{
    switch (tabIndex) {
        case 0: return ui->intro_md;
        case 1: return ui->registro_md;
        case 2: return ui->map_md;
        case 3: return ui->prob_md;
        case 4: return ui->stats_md;
        default: return nullptr;
    }
}

QTreeWidget* Help::getTreeForTab(int tabIndex)
{
    switch (tabIndex) {
        case 0: return ui->intro_index;
        case 1: return ui->registro_index;
        case 2: return ui->map_index;
        case 3: return ui->prob_index;
        case 4: return ui->stats_index;
        default: return nullptr;
    }
}

QString Help::getTabName(int tabIndex)
{
    switch (tabIndex) {
        case 0: return "intro";
        case 1: return "registro";
        case 2: return "map";
        case 3: return "prob";
        case 4: return "stats";
        default: return "";
    }
}
