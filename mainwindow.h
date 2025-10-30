#pragma once
#include <QMainWindow>
#include <QWidgetAction>
#include "pixelcanvas.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void clearCanvas();
    void setStepAlg();
    void setDDAAlg();
    void setBresenhamAlg();
    void setCircleAlg();
    void showTimingComparison();
    void triggerUndo();
    void triggerRedo();

private:
    PixelCanvas *canvas;
    void createMenu();
    QWidgetAction* createColoredAction(const QString& text, const QColor& color, QObject* receiver, const char* slot);

};
