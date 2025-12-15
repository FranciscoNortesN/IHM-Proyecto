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
    explicit SelecPro(NavigationDAO *dao, QWidget *parent = nullptr);
    ~SelecPro();

private slots:
    void onRandomButtonClicked();
    void onProblemSelected();

private:
    Ui::SelecPro *ui;
    NavigationDAO *m_dao;
    QVector<Problem> m_problems;
    
    void loadProblems();
    void createSampleProblems();
};

#endif // SELECPRO_H
