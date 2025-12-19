#ifndef SELECPRO_H
#define SELECPRO_H

#include <QWidget>
#include "navlib/navigationdao.h"
#include "navlib/navtypes.h"
#include "navlib/navdaoexception.h"

namespace Ui {
class SelecPro;
}

class SelecPro : public QWidget
{
    Q_OBJECT

public:
    explicit SelecPro(NavigationDAO *dao, const QString &userNickname, QWidget *parent = nullptr);
    ~SelecPro();
    
    void setMainWindow(class MainWindow *mainWindow);

private slots:
    void onRandomButtonClicked();
    void onProblemSelected();

private:
    Ui::SelecPro *ui;
    NavigationDAO *m_dao;
    QString m_userNickname;
    QVector<Problem> m_problems;
    class MainWindow *m_mainWindow = nullptr;
    
    void loadProblems();
    void createSampleProblems();
};

#endif // SELECPRO_H
