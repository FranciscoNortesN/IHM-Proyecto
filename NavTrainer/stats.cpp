#include "stats.h"
#include "ui_stats.h"
#include "navlib/navigationdao.h"
#include "navlib/navdaoexception.h"
#include <QMessageBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QDateTime>
#include <QMap>

Stats::Stats(NavigationDAO *dao, const QString &userNickname, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Stats)
    , m_dao(dao)
    , m_userNickname(userNickname)
{
    ui->setupUi(this);
    
    // Configurar tabla
    ui->sessionsTable->horizontalHeader()->setStretchLastSection(false);
    ui->sessionsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->sessionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    // Configurar anchos de columnas
    ui->sessionsTable->setColumnWidth(0, 80);  // Sesión
    ui->sessionsTable->setColumnWidth(1, 200); // Inicio - Fin
    ui->sessionsTable->setColumnWidth(2, 100); // Duración
    ui->sessionsTable->setColumnWidth(3, 70);  // Aciertos
    ui->sessionsTable->setColumnWidth(4, 70);  // Fallos
    
    loadUserStats();
}

Stats::~Stats()
{
    delete ui;
}

void Stats::loadUserStats()
{
    if (!m_dao || m_userNickname.isEmpty()) {
        ui->label_2->setText("0");
        ui->label_3->setText("0");
        ui->label_4->setText("0");
        return;
    }
    
    try {
        QVector<Session> sessions = m_dao->loadSessionsFor(m_userNickname);
        
        int totalHits = 0;
        int totalFaults = 0;
        
        // Llenar la tabla - cada sesión es una fila
        ui->sessionsTable->setRowCount(sessions.size());
        
        for (int i = 0; i < sessions.size(); ++i) {
            const Session &session = sessions[i];
            
            totalHits += session.hits();
            totalFaults += session.faults();
            
            QDateTime startTime = session.timeStamp();
            // Asumimos que cada sesión dura aproximadamente el tiempo entre problemas
            // Para simplificar, mostramos solo el tiempo de inicio
            QString timeRange = startTime.toString("dd/MM hh:mm");
            
            ui->sessionsTable->setItem(i, 0, new QTableWidgetItem(QString("Sesión %1").arg(i + 1)));
            ui->sessionsTable->setItem(i, 1, new QTableWidgetItem(timeRange));
            ui->sessionsTable->setItem(i, 2, new QTableWidgetItem("-")); // Duración no disponible por sesión individual
            ui->sessionsTable->setItem(i, 3, new QTableWidgetItem(QString::number(session.hits())));
            ui->sessionsTable->setItem(i, 4, new QTableWidgetItem(QString::number(session.faults())));
            
            // Centrar contenido de todas las celdas
            for (int col = 0; col < 5; ++col) {
                ui->sessionsTable->item(i, col)->setTextAlignment(Qt::AlignCenter);
            }
        }
        
        // Actualizar contadores
        ui->label_2->setText(QString::number(totalHits));
        ui->label_3->setText(QString::number(totalFaults));
        
        int total = totalHits + totalFaults;
        int percentage = (total > 0) ? (totalHits * 100 / total) : 0;
        ui->label_4->setText(QString::number(percentage));
        
    } catch (const NavDAOException &e) {
        QMessageBox::critical(this, tr("Error"), 
            tr("Error al cargar estadísticas: %1").arg(e.what()));
        
        ui->label_2->setText("0");
        ui->label_3->setText("0");
        ui->label_4->setText("0");
    }
}
