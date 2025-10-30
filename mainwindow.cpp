#include "mainwindow.h"
#include <QMenuBar>
#include <QAction>
#include <QStatusBar>
#include <QMessageBox>
#include <QFile>
#include <QApplication>
#include <QWidgetAction>
#include <QPushButton>
#include <QObject>
#include <QHBoxLayout>
#include <QShortcut>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    canvas = new PixelCanvas(this);
    setCentralWidget(canvas);
    resize(1000, 800);
    setWindowTitle("Raster Algorithms Visualizer");

    QFile styleFile(":/style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString style = QLatin1String(styleFile.readAll());
        qApp->setStyleSheet(style);
    }

    createMenu();

    connect(canvas, &PixelCanvas::cursorPositionChanged, this, [this](QPoint pt) {
        statusBar()->showMessage(QString("X: %1, Y: %2").arg(pt.x()).arg(pt.y()));
    });
}
void MainWindow::createMenu() {
    // === Меню "Файл" ===
    QMenu *fileMenu = menuBar()->addMenu("Файл");

    QAction *clearAct = new QAction("Очистить", this);
    connect(clearAct, &QAction::triggered, this, &MainWindow::clearCanvas);
    fileMenu->addAction(clearAct);

    // создаём QShortcut вручную (работает, но не отображается в меню)
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_L), this, SLOT(clearCanvas()));

    QAction *undoAct = new QAction("Отменить", this);
    connect(undoAct, &QAction::triggered, canvas, &PixelCanvas::undo);
    fileMenu->addAction(undoAct);
    new QShortcut(QKeySequence::Undo, this, SLOT(undo()));

    QAction *redoAct = new QAction("Повторить", this);
    connect(redoAct, &QAction::triggered, canvas, &PixelCanvas::redo);
    fileMenu->addAction(redoAct);
    new QShortcut(QKeySequence::Redo, this, SLOT(redo()));


    // === Меню "Алгоритмы" ===
    QMenu *algMenu = menuBar()->addMenu("Алгоритмы");

    algMenu->addAction(createColoredAction("Пошаговый", QColor("#7FB7E8"), this, SLOT(setStepAlg())));
    algMenu->addAction(createColoredAction("ЦДА", QColor("#A6C48A"), this, SLOT(setDDAAlg())));
    algMenu->addAction(createColoredAction("Брезенхем (отрезок)", QColor("#C8A5D4"), this, SLOT(setBresenhamAlg())));
    algMenu->addAction(createColoredAction("Брезенхем (окружность)", QColor("#F4A261"), this, SLOT(setCircleAlg())));


    // === Меню "Анализ" ===
    QMenu *analysisMenu = menuBar()->addMenu("Анализ");
    QAction *compareAction = new QAction("Сравнение времени работы", this);
    connect(compareAction, &QAction::triggered, this, &MainWindow::showTimingComparison);
    analysisMenu->addAction(compareAction);
}


void MainWindow::clearCanvas() {
    canvas->clear();
}

void MainWindow::setStepAlg() {
    canvas->setAlgorithm(AlgorithmType::Step);
    statusBar()->showMessage("Выбран: Пошаговый алгоритм");
}
void MainWindow::setDDAAlg() {
    canvas->setAlgorithm(AlgorithmType::DDA);
    statusBar()->showMessage("Выбран: Алгоритм ЦДА");
}
void MainWindow::setBresenhamAlg() {
    canvas->setAlgorithm(AlgorithmType::Bresenham);
    statusBar()->showMessage("Выбран: Алгоритм Брезенхема (отрезок)");
}
void MainWindow::setCircleAlg() {
    canvas->setAlgorithm(AlgorithmType::Circle);
    statusBar()->showMessage("Выбран: Алгоритм Брезенхема (окружность)");
}

void MainWindow::showTimingComparison() {
    QString text = canvas->getAverageTimes();
    QMessageBox::information(this, "Сравнение времени алгоритмов", text);
}


QWidgetAction* MainWindow::createColoredAction(const QString& text, const QColor& color, QObject* receiver, const char* slot)
{
    auto *button = new QPushButton(text, this);
    button->setStyleSheet(QString(
                              "QPushButton {"
                              " background-color: %1;"
                              " color: #5C4B42;"
                              " border: none;"
                              " padding: 6px 12px;"
                              " text-align: left;"
                              " border-radius: 4px;"
                              " font-weight: 500;"
                              "}"
                              "QPushButton:hover {"
                              " background-color: %2;"
                              "}"
                              )
                              .arg(color.name())
                              .arg(color.darker(115).name()));

    auto *container = new QWidget(this);
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(button);

    auto *action = new QWidgetAction(this);
    action->setDefaultWidget(container);

    // подключаем слот
    QObject::connect(button, SIGNAL(clicked()), receiver, slot);

    // закрываем меню после клика
    QObject::connect(button, &QPushButton::clicked, [button]() {
        QWidget *p = button->parentWidget();
        while (p) {
            if (auto *menu = qobject_cast<QMenu*>(p)) {
                menu->close();
                break;
            }
            p = p->parentWidget();
        }
    });

    return action;
}


void MainWindow::triggerUndo() {
    canvas->undo();
}

void MainWindow::triggerRedo() {
    canvas->redo();
}
