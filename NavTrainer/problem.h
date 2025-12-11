#ifndef PROBLEM_H
#define PROBLEM_H

#include <QWidget>

namespace Ui {
class problem;
}

class Problem : public QWidget
{
    Q_OBJECT

public:
    explicit Problem(QWidget *parent = nullptr);
    ~Problem();

private:
    Ui::problem *ui;
};

#endif // PROBLEM_H
