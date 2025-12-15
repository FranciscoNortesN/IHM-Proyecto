#ifndef PROBLEM_H
#define PROBLEM_H

#include <QWidget>
#include <QButtonGroup>
#include "navlib/navtypes.h"

namespace Ui {
class problem;
}

class ProblemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProblemWidget(const Problem &problem, QWidget *parent = nullptr);
    ~ProblemWidget();

private slots:
    void onCheckButtonClicked();

private:
    Ui::problem *ui;
    Problem m_problem;
    QButtonGroup *m_buttonGroup;
    
    void setupProblem();
};

#endif // PROBLEM_H
