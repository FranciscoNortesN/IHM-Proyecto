#include "problem.h"
#include "ui_problem.h"
#include "NavigationDAO.h"
#include "NavDAOException.h"
#include <QMessageBox>
#include <QRadioButton>

ProblemWidget::ProblemWidget(const Problem &problem, NavigationDAO *dao, const QString &userNickname, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::problem),
    m_problem(problem),
    m_dao(dao),
    m_userNickname(userNickname)
{
    ui->setupUi(this);
    
    // Configurar QButtonGroup
    m_buttonGroup = new QButtonGroup(this);
    m_buttonGroup->addButton(ui->radioButton, 0);
    m_buttonGroup->addButton(ui->radioButton_2, 1);
    m_buttonGroup->addButton(ui->radioButton_3, 2);
    m_buttonGroup->addButton(ui->radioButton_4, 3);
    
    connect(ui->pushButton, &QPushButton::clicked, this, &ProblemWidget::onCheckButtonClicked);
    
    setupProblem();
}

ProblemWidget::~ProblemWidget()
{
    delete ui;
}

void ProblemWidget::setupProblem()
{
    // Configurar tamaño y propiedades de la ventana
    setMinimumSize(400, 300);
    resize(500, 400);
    
    // Mostrar el texto del problema con ajuste de línea
    ui->label->setText(m_problem.text());
    ui->label->setWordWrap(true);
    ui->label->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    
    // Configurar las respuestas
    QVector<Answer> answers = m_problem.answers();
    QList<QRadioButton*> radioButtons = {ui->radioButton, ui->radioButton_2, ui->radioButton_3, ui->radioButton_4};
    

    
    for (int i = 0; i < radioButtons.size(); ++i) {
        if (i < answers.size() && !answers[i].text().isEmpty()) {
            radioButtons[i]->setText(answers[i].text());
            radioButtons[i]->setProperty("isCorrect", answers[i].validity());
            radioButtons[i]->setVisible(true);
        } else {
            radioButtons[i]->setVisible(false);
        }
    }
}

void ProblemWidget::onCheckButtonClicked()
{
    int selectedIndex = m_buttonGroup->checkedId();
    
    if (selectedIndex == -1) {
        QMessageBox::information(this, tr("Información"), tr("Por favor, selecciona una respuesta"));
        return;
    }
    
    QRadioButton* selectedButton = qobject_cast<QRadioButton*>(m_buttonGroup->button(selectedIndex));
    bool isCorrect = selectedButton->property("isCorrect").toBool();
    
    // Deshabilitar todos los radioButtons para evitar cambios
    QList<QAbstractButton*> buttons = m_buttonGroup->buttons();
    for (QAbstractButton* btn : buttons) {
        btn->setEnabled(false);
    }
    
    if (isCorrect) {
        // Sombreado verde para respuesta correcta
        selectedButton->setStyleSheet("QRadioButton { background-color: #90EE90; padding: 5px; }");
    } else {
        // Sombreado rojo para respuesta incorrecta
        selectedButton->setStyleSheet("QRadioButton { background-color: #FFB6C1; padding: 5px; }");
    }
    
    // Registrar resultado en estadísticas
    recordResult(isCorrect);
    
    // Deshabilitar el botón de comprobar
    ui->pushButton->setEnabled(false);
}

void ProblemWidget::recordResult(bool isCorrect)
{
    if (!m_dao || m_userNickname.isEmpty()) {
        return;
    }
    
    try {
        QString problemText = isCorrect ? QString() : m_problem.text();
        Session session(QDateTime::currentDateTime(), isCorrect ? 1 : 0, isCorrect ? 0 : 1, problemText);
        m_dao->addSession(m_userNickname, session);
    } catch (const NavDAOException &e) {
        qWarning() << "Error al registrar estadísticas:" << e.what();
    }
}
