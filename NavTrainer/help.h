#ifndef HELP_H
#define HELP_H

#include <QWidget>
#include <QTreeWidgetItem>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QMap>
#include <QList>

namespace Ui {
class Help;
}

struct SearchResult {
    int tabIndex;           // Índice del tab donde está el resultado
    QTextCursor cursor;     // Cursor posicionado en el resultado
    QString context;        // Contexto del resultado
};

class Help : public QWidget
{
    Q_OBJECT

public:
    explicit Help(QWidget *parent = nullptr);
    ~Help();

private slots:
    void onSearchButtonClicked();
    void onNextButtonClicked();
    void onPrevButtonClicked();
    void onCloseBannerClicked();
    void onIndexItemClicked(QTreeWidgetItem *item, int column);

private:
    Ui::Help *ui;
    
    // Almacena los resultados de búsqueda
    QList<SearchResult> searchResults;
    int currentResultIndex;
    QString lastSearchTerm;
    
    // Mapeo de secciones para el índice
    struct Section {
        QString title;
        QString anchor;
        int position;
    };
    
    QMap<QString, QList<Section>> sectionsByTab;
    
    // Métodos privados
    void loadMarkdownContent();
    void loadMarkdownFile(const QString &resourcePath, QTextBrowser *browser);
    void setupIndex();
    void parseMarkdownHeaders(const QString &content, const QString &tabName);
    void populateTreeWidget(QTreeWidget *tree, const QString &tabName);
    
    void performSearch(const QString &searchTerm);
    void highlightSearchResult(const SearchResult &result);
    void clearHighlights();
    void showBanner(bool show);
    void updateBannerText(const QString &searchTerm);
    
    QString markdownToHtml(const QString &markdown);
    QTextBrowser* getBrowserForTab(int tabIndex);
    QTreeWidget* getTreeForTab(int tabIndex);
    QString getTabName(int tabIndex);
};

#endif // HELP_H
