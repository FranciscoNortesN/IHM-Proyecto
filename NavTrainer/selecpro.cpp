#include "selecpro.h"
#include "ui_selecpro.h"
#include "navlib/navdaoexception.h"
#include "problem.h"
#include <QMessageBox>
#include <QListWidgetItem>
#include <QRandomGenerator>
#include <QSize>

SelecPro::SelecPro(NavigationDAO *dao, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SelecPro)
    , m_dao(dao)
{
    ui->setupUi(this);
    
    connect(ui->pushButton, &QPushButton::clicked, this, &SelecPro::onRandomButtonClicked);
    connect(ui->listWidget, &QListWidget::itemDoubleClicked, this, &SelecPro::onProblemSelected);
    
    // Configurar apariencia de la lista
    ui->listWidget->setStyleSheet(
        "QListWidget {"
        "    outline: none;"
        "}"
        "QListWidget::item {"
        "    border: 2px solid #cccccc;"
        "    border-radius: 8px;"
        "    padding: 15px;"
        "    margin: 5px;"
        "    background-color: #f9f9f9;"
        "    font-size: 14px;"
        "    color: #000000;"
        "}"
        "QListWidget::item:hover {"
        "    border-color: #0078d4;"
        "    background-color: #e6f3ff;"
        "    color: #000000;"
        "}"
        "QListWidget::item:selected {"
        "    border-color: #0078d4;"
        "    background-color: #cce7ff;"
        "    color: #000000;"
        "}"
        "QListWidget::item:focus {"
        "    outline: none;"
        "}"
    );
    
    loadProblems();
}

SelecPro::~SelecPro()
{
    delete ui;
}

void SelecPro::loadProblems()
{
    if (!m_dao) {
        QMessageBox::warning(this, tr("Error"), tr("No hay conexión a la base de datos"));
        return;
    }
    
    try {
        m_problems = m_dao->loadProblems();
        
        ui->listWidget->clear();
        
        // Los problemas se cargan desde la base de datos real
        
        for (int i = 0; i < m_problems.size(); ++i) {
            const Problem &problem = m_problems[i];
            QString displayText = QString("Ejercicio %1: %2")
                .arg(i + 1)
                .arg(problem.text().left(50) + (problem.text().length() > 50 ? "..." : ""));
            
            QListWidgetItem *item = new QListWidgetItem(displayText);
            item->setData(Qt::UserRole, i);
            item->setSizeHint(QSize(0, 80)); // Altura fija de 80px
            ui->listWidget->addItem(item);
        }
        
        if (m_problems.isEmpty()) {
            QMessageBox::information(this, tr("Información"), 
                tr("No se encontraron ejercicios en la base de datos"));
        }
        
    } catch (const NavDAOException &e) {
        QMessageBox::critical(this, tr("Error"), 
            tr("Error al cargar ejercicios: %1").arg(e.what()));
    }
}

void SelecPro::createSampleProblems()
{
    QVector<Problem> sampleProblems;
    
    QVector<Answer> answers1;
    answers1.append(Answer("Norte", true));
    answers1.append(Answer("Sur", false));
    answers1.append(Answer("Este", false));
    answers1.append(Answer("Oeste", false));
    sampleProblems.append(Problem("¿Hacia dónde apunta la aguja de una brújula?", answers1));
    
    QVector<Answer> answers2;
    answers2.append(Answer("0°", false));
    answers2.append(Answer("90°", true));
    answers2.append(Answer("180°", false));
    answers2.append(Answer("270°", false));
    sampleProblems.append(Problem("¿Cuántos grados hay en un ángulo recto?", answers2));
    
    QVector<Answer> answers3;
    answers3.append(Answer("Latitud", false));
    answers3.append(Answer("Longitud", false));
    answers3.append(Answer("Rumbo", true));
    answers3.append(Answer("Distancia", false));
    sampleProblems.append(Problem("¿Cómo se llama la dirección de navegación?", answers3));
    
    try {
        m_dao->replaceAllProblems(sampleProblems);
    } catch (const NavDAOException &e) {
        QMessageBox::critical(this, tr("Error"), 
            tr("Error al crear ejercicios de prueba: %1").arg(e.what()));
    }
}

void SelecPro::onRandomButtonClicked()
{
    if (m_problems.isEmpty()) {
        QMessageBox::information(this, tr("Información"), tr("No hay ejercicios disponibles"));
        return;
    }
    
    int randomIndex = QRandomGenerator::global()->bounded(m_problems.size());
    ui->listWidget->setCurrentRow(randomIndex);
    
    // Abrir directamente la ventana del problema aleatorio
    const Problem &selectedProblem = m_problems[randomIndex];
    ProblemWidget *problemWindow = new ProblemWidget(selectedProblem, this);
    problemWindow->setAttribute(Qt::WA_DeleteOnClose);
    problemWindow->setWindowFlags(Qt::Window);
    problemWindow->setWindowTitle(tr("Ejercicio %1 (Aleatorio)").arg(randomIndex + 1));
    problemWindow->show();
}

void SelecPro::onProblemSelected()
{
    QListWidgetItem *currentItem = ui->listWidget->currentItem();
    if (!currentItem) {
        return;
    }
    
    int problemIndex = currentItem->data(Qt::UserRole).toInt();
    if (problemIndex >= 0 && problemIndex < m_problems.size()) {
        const Problem &selectedProblem = m_problems[problemIndex];
        
        ProblemWidget *problemWindow = new ProblemWidget(selectedProblem, this);
        problemWindow->setAttribute(Qt::WA_DeleteOnClose);
        problemWindow->setWindowFlags(Qt::Window);
        problemWindow->setWindowTitle(tr("Ejercicio %1").arg(problemIndex + 1));
        problemWindow->show();
    }
}
