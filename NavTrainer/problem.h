#ifndef PROBLEM_H
#define PROBLEM_H

#include <QWidget>
#include <QButtonGroup>
#include "navlib/navtypes.h"

class NavigationDAO;

namespace Ui {
class problem;
}

class ProblemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProblemWidget(const Problem &problem, NavigationDAO *dao, const QString &userNickname, QWidget *parent = nullptr);
    ~ProblemWidget();

private slots:
    void onCheckButtonClicked();

private:
    Ui::problem *ui;
    Problem m_problem;
    QButtonGroup *m_buttonGroup;
    NavigationDAO *m_dao;
    QString m_userNickname;
    
    void setupProblem();
    void recordResult(bool isCorrect);
};

#endif // PROBLEM_H
