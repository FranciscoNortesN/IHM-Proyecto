#include "stats.h"
#include "ui_stats.h"
#include "navlib/navigationdao.h"
#include "navlib/navdaoexception.h"
#include <QMessageBox>

Stats::Stats(NavigationDAO *dao, const QString &userNickname, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Stats)
    , m_dao(dao)
    , m_userNickname(userNickname)
{
    ui->setupUi(this);
    loadUserStats();
}

Stats::~Stats()
{
    delete ui;
}

void Stats::loadUserStats()
{
    if (!m_dao || m_userNickname.isEmpty()) {
        // Sin usuario, mostrar valores por defecto
        ui->label_2->setText("0");
        ui->label_3->setText("0");
        ui->label_4->setText("0");
        return;
    }
    
    try {
        QVector<Session> sessions = m_dao->loadSessionsFor(m_userNickname);
        
        int totalHits = 0;
        int totalFaults = 0;
        
        ui->listWidget->clear();   // Aciertos
        ui->listWidget_2->clear(); // Fallos
        
        for (const Session &session : sessions) {
            totalHits += session.hits();
            totalFaults += session.faults();
            
            QString timeStr = session.timeStamp().toString("dd/MM/yyyy hh:mm");
            
            if (session.hits() > 0) {
                ui->listWidget->addItem(QString("Acierto - %1").arg(timeStr));
            }
            if (session.faults() > 0) {
                QString problemText = session.problemText().isEmpty() ? "Problema no registrado" : session.problemText();
                ui->listWidget_2->addItem(QString("Fallo - %1\n%2").arg(timeStr, problemText));
            }
        }
        
        // Actualizar contadores
        ui->label_2->setText(QString::number(totalHits));
        ui->label_3->setText(QString::number(totalFaults));
        
        // Calcular porcentaje
        int total = totalHits + totalFaults;
        int percentage = (total > 0) ? (totalHits * 100 / total) : 0;
        ui->label_4->setText(QString::number(percentage));
        
    } catch (const NavDAOException &e) {
        QMessageBox::critical(this, tr("Error"), 
            tr("Error al cargar estadísticas: %1").arg(e.what()));
        
        // Mostrar valores por defecto en caso de error
        ui->label_2->setText("0");
        ui->label_3->setText("0");
        ui->label_4->setText("0");
    }
}
