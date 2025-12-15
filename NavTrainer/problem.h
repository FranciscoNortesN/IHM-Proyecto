#ifndef PROBLEM_H
#define PROBLEM_H

#include <QWidget>

namespace Ui {
class problem;
}

class ProblemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProblemWidget(QWidget *parent = nullptr);
    ~ProblemWidget();

private:
    Ui::problem *ui;
};

#endif // PROBLEM_H
