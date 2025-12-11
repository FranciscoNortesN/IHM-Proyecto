#ifndef SELECPRO_H
#define SELECPRO_H

#include <QWidget>

namespace Ui {
class SelecPro;
}

class SelecPro : public QWidget
{
    Q_OBJECT

public:
    explicit SelecPro(QWidget *parent = nullptr);
    ~SelecPro();

private:
    Ui::SelecPro *ui;
};

#endif // SELECPRO_H
