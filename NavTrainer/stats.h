#ifndef STATS_H
#define STATS_H

#include <QWidget>
#include "navlib/navtypes.h"

class NavigationDAO;

namespace Ui {
class Stats;
}

class Stats : public QWidget
{
    Q_OBJECT

public:
    explicit Stats(NavigationDAO *dao, const QString &userNickname, QWidget *parent = nullptr);
    ~Stats();

private:
    Ui::Stats *ui;
    NavigationDAO *m_dao;
    QString m_userNickname;
    
    void loadUserStats();
};

#endif // STATS_H
