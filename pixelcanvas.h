#pragma once
#include <QWidget>
#include <QColor>
#include <QPoint>
#include <QHash>
#include <QStack>
#include <QMap>
#include <QDebug>


enum class AlgorithmType { None, Step, DDA, Bresenham, Circle };


inline uint qHash(const QPoint &key, uint seed = 0) noexcept {
    return qHash(QPair<int,int>(key.x(), key.y()), seed);
}

class PixelCanvas : public QWidget {
    Q_OBJECT
public:
    explicit PixelCanvas(QWidget *parent = nullptr);

    void clear();
    void setAlgorithm(AlgorithmType a) { currentAlg = a; }
    int  getZoom() const { return int(cellSize); }
    void setZoom(int v);                // дискретный шаг увеличения
    QString getAverageTimes() const;


public slots:
    void undo();
    void redo();

signals:
    void cursorPositionChanged(QPoint gridPos); // логические координаты (центр = 0,0)

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;

private:
    // === состояние «бесконечного» холста ===
    qreal cellSize = 12.0;              // размер клетки (экранных пикселей)
    QPointF panPx {0,0};                // смещение холста в пикселях
    AlgorithmType currentAlg = AlgorithmType::None;

    // нарисованные пиксели: ключ = целочисленные координаты сетки
    QHash<QPoint, QColor> pixels;


    QStack<QHash<QPoint, QColor>> undoStack;
    QStack<QHash<QPoint, QColor>> redoStack;


    void saveState();

    // взаимодействие
    bool panning = false;
    QPoint lastMouse;
    bool waitingSecond = false;
    QPoint firstPt;


    // преобразования координат
    QPointF originPx() const;                   // центр виджета
    QPointF gridToScreenF(QPointF g) const;     // логические -> экран
    QPoint   gridToScreen(QPoint g) const;
    QPointF screenToGridF(QPointF s) const;     // экран -> логические (вещественные)
    QPoint   screenToGrid(QPoint s) const;      // экран -> целочисленные (по полу)

    // пиксельная запись/отрисовка
    void setPixel(QPoint g, const QColor& c = Qt::black);

    // алгоритмы
    void drawLineStep(QPoint a, QPoint b);
    void drawLineDDA(QPoint a, QPoint b);
    void drawLineBresenham(QPoint a, QPoint b);
    void drawCircleBresenham(QPoint c, int r);


    // --- статистика времени ---
    QVector<qreal> timesStep;
    QVector<qreal> timesDDA;
    QVector<qreal> timesBresenhamLine;
    QVector<qreal> timesBresenhamCircle;


};
