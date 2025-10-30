#include "pixelcanvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <cmath>
#include <algorithm>
#include <QElapsedTimer>
#include <QDebug>


PixelCanvas::PixelCanvas(QWidget *parent) : QWidget(parent) {
    setMouseTracking(true);
    setMinimumSize(800, 600);
}

void PixelCanvas::clear() { pixels.clear(); update(); }

void PixelCanvas::setZoom(int v) {
    cellSize = std::clamp<qreal>(v, 4.0, 64.0);
    update();
}

// ---------- координаты ----------
QPointF PixelCanvas::originPx() const { return QPointF(width()/2.0, height()/2.0); }

QPointF PixelCanvas::gridToScreenF(QPointF g) const {
    // y вверх положительный в «сетке»
    return originPx() + panPx + QPointF(g.x()*cellSize, -g.y()*cellSize);
}
QPoint PixelCanvas::gridToScreen(QPoint g) const { return gridToScreenF(QPointF(g)).toPoint(); }

QPointF PixelCanvas::screenToGridF(QPointF s) const {
    QPointF v = s - originPx() - panPx;
    return QPointF(v.x()/cellSize, -v.y()/cellSize);
}
QPoint PixelCanvas::screenToGrid(QPoint s) const {
    QPointF g = screenToGridF(QPointF(s));
    return QPoint(std::floor(g.x()), std::floor(g.y()));
}

void PixelCanvas::setPixel(QPoint g, const QColor& c) {
    pixels[g] = c;
}

static QColor algorithmColor(AlgorithmType type) {
    switch (type) {
    case AlgorithmType::Step:      return QColor(0, 120, 255);   // синий
    case AlgorithmType::DDA:       return QColor(0, 180, 0);     // зелёный
    case AlgorithmType::Bresenham: return QColor(160, 0, 255);   // фиолетовый
    case AlgorithmType::Circle:    return QColor(255, 140, 0) ;  // красный
    default:                       return Qt::black;
    }
}

// ---------- вспомогательная функция ----------
// Функция подбирает "красивый" шаг делений в зависимости от cellSize
static int computeTickStep(qreal cellSize) {
    qreal targetPx = 80.0;
    qreal logicalStep = targetPx / cellSize;
    qreal pow10 = std::pow(10.0, std::floor(std::log10(logicalStep)));
    qreal norm = logicalStep / pow10;
    qreal nice;
    if (norm < 1.5) nice = 1;
    else if (norm < 3.5) nice = 2;
    else if (norm < 7.5) nice = 5;
    else nice = 10;
    return int(nice * pow10);
}

void PixelCanvas::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), Qt::white);

    // --- адаптивная сетка ---
    p.save();

    int coarseStep = 1;
    if (cellSize < 2)       coarseStep = 50;
    else if (cellSize < 4)  coarseStep = 20;
    else if (cellSize < 8)  coarseStep = 10;
    else if (cellSize < 16) coarseStep = 5;
    else if (cellSize < 32) coarseStep = 2;

    QColor fineColor(230,230,230);
    QColor boldColor(200,200,200);

    QPointF gLT = screenToGridF(QPointF(0,0));
    QPointF gRB = screenToGridF(QPointF(width(),height()));
    int gxMin = std::floor(std::min(gLT.x(), gRB.x()));
    int gxMax = std::ceil (std::max(gLT.x(), gRB.x()));
    int gyMin = std::floor(std::min(gLT.y(), gRB.y()));
    int gyMax = std::ceil (std::max(gLT.y(), gRB.y()));

    for (int gx = gxMin; gx <= gxMax; ++gx) {
        QPointF s1 = gridToScreenF(QPointF(gx, gyMin));
        QPointF s2 = gridToScreenF(QPointF(gx, gyMax));
        if (gx % coarseStep == 0)
            p.setPen(QPen(boldColor, 1));
        else if (cellSize > 6)
            p.setPen(QPen(fineColor, 1));
        else
            continue;
        p.drawLine(s1, s2);
    }

    for (int gy = gyMin; gy <= gyMax; ++gy) {
        QPointF s1 = gridToScreenF(QPointF(gxMin, gy));
        QPointF s2 = gridToScreenF(QPointF(gxMax, gy));
        if (gy % coarseStep == 0)
            p.setPen(QPen(boldColor, 1));
        else if (cellSize > 6)
            p.setPen(QPen(fineColor, 1));
        else
            continue;
        p.drawLine(s1, s2);
    }

    p.restore();

    // --- оси координат ---
    p.save();
    QPen axisPen(QColor(50,50,50), 2);
    p.setPen(axisPen);

    const qreal ox = originPx().x() + panPx.x();
    const qreal oy = originPx().y() + panPx.y();

    p.drawLine(QPointF(0, oy), QPointF(width(), oy));   // X
    p.drawLine(QPointF(ox, 0), QPointF(ox, height()));  // Y
    p.restore();

    // --- подписи делений ---
    p.save();
    p.setPen(Qt::black);
    QFont font = p.font();
    font.setPointSize(8);
    p.setFont(font);

    const int tickStep = computeTickStep(cellSize);

    for (int gx = gxMin; gx <= gxMax; ++gx) {
        if (gx % tickStep == 0) {
            QPoint sp = gridToScreen(QPoint(gx, 0));
            if (gx != 0)
                p.drawText(sp.x()+2, oy-2, QString::number(gx));
        }
    }

    for (int gy = gyMin; gy <= gyMax; ++gy) {
        if (gy % tickStep == 0) {
            QPoint sp = gridToScreen(QPoint(0, gy));
            if (gy != 0)
                p.drawText(ox+4, sp.y()-2, QString::number(gy));
            else
                p.drawText(ox+6, sp.y()-2, "0"); // один нолик в центре
        }
    }
    p.restore();

    // --- Подсветка первой точки при ожидании второй ---
    if (waitingSecond) {
        QPoint s = gridToScreen(firstPt);
        QRect r(s.x(), s.y(), int(std::ceil(cellSize)), int(std::ceil(cellSize)));
        p.setBrush(Qt::red);
        p.setPen(Qt::black);
        p.drawRect(r);
    }


    // --- подсветка клетки под курсором ---
    QPoint cursor = mapFromGlobal(QCursor::pos());
    if (rect().contains(cursor)) {
        QPoint g = screenToGrid(cursor);
        QPoint s = gridToScreen(g);
        QRect r(s.x(), s.y(), int(std::ceil(cellSize)), int(std::ceil(cellSize)));
        p.fillRect(r, QColor(200,200,200,60));
        p.setPen(QPen(Qt::gray,1,Qt::DashLine));
        p.drawRect(r);
    }

    // --- отрисовка пикселей ---
    p.setPen(Qt::NoPen);
    for (auto it = pixels.constBegin(); it != pixels.constEnd(); ++it) {
        const QPoint& g = it.key();
        if (g.x() < gxMin-1 || g.x() > gxMax+1 || g.y() < gyMin-1 || g.y() > gyMax+1) continue;
        QPoint s = gridToScreen(g);
        QRect r(s.x(), s.y(), int(std::ceil(cellSize)), int(std::ceil(cellSize)));
        p.fillRect(r, it.value());
    }
}


// ---------- мышь / зум ----------
void PixelCanvas::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::RightButton) { panning = true; lastMouse = e->pos(); return; }

    if (e->button() == Qt::LeftButton) {
        // если алгоритм не выбран — игнорируем клик
        if (currentAlg == AlgorithmType::None)
            return;

        QPoint g = screenToGrid(e->pos());

        // первая точка — просто сохраняем и подсвечиваем
        if (!waitingSecond) {
            firstPt = g;
            waitingSecond = true;
            update();
        }
        // вторая точка — рисуем и сбрасываем подсветку
        else {
            QPoint b = g;
            waitingSecond = false;

            saveState();
            switch (currentAlg) {
            case AlgorithmType::Step:      drawLineStep(firstPt, b); break;
            case AlgorithmType::DDA:       drawLineDDA(firstPt, b); break;
            case AlgorithmType::Bresenham: drawLineBresenham(firstPt, b); break;
            case AlgorithmType::Circle: {
                int dx = b.x() - firstPt.x();
                int dy = b.y() - firstPt.y();
                drawCircleBresenham(firstPt, int(std::lround(std::sqrt(dx*dx + dy*dy))));
                break;
            }
            default: break;
            }

            update();
        }
    }

}

void PixelCanvas::mouseMoveEvent(QMouseEvent *e) {
    if (panning) {
        panPx += (e->pos() - lastMouse);
        lastMouse = e->pos();
        update();
    } else {
        emit cursorPositionChanged(screenToGrid(e->pos()));
        update(); // чтобы подсветка следовала
    }
}
void PixelCanvas::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() == Qt::RightButton) panning = false;
}
void PixelCanvas::wheelEvent(QWheelEvent *e) {
    // зум к курсору: сохраняем логическую координату под курсором
    QPointF s = e->position();
    QPointF gBefore = screenToGridF(s);

    cellSize = std::clamp(cellSize * (e->angleDelta().y() > 0 ? 1.1 : 0.9), 4.0, 64.0);

    // смещение так, чтобы та же логическая точка осталась под курсором
    QPointF desiredScreen = originPx() + panPx + QPointF(gBefore.x()*cellSize, -gBefore.y()*cellSize);
    panPx += (s - desiredScreen);
    update();
}


QString PixelCanvas::getAverageTimes() const {
    auto avg = [](const QVector<qreal>& v) {
        if (v.isEmpty()) return 0.0;
        qreal sum = 0;
        for (auto t : v) sum += t;
        return sum / v.size();
    };

    QString text;
    text += QString("Среднее время Step: %1 мс\n").arg(avg(timesStep), 0, 'f', 3);
    text += QString("Среднее время DDA: %1 мс\n").arg(avg(timesDDA), 0, 'f', 3);
    text += QString("Среднее время Брезенхема (отрезок): %1 мс\n").arg(avg(timesBresenhamLine), 0, 'f', 3);
    text += QString("Среднее время Брезенхема (окружность): %1 мс").arg(avg(timesBresenhamCircle), 0, 'f', 3);

    return text;
}


// ---------- Undo / Redo ----------

void PixelCanvas::saveState() {
    undoStack.push(pixels);    // сохраняем текущее состояние
    redoStack.clear();         // очищаем redo при новом действии
}

void PixelCanvas::undo() {
    if (!undoStack.isEmpty()) {
        redoStack.push(pixels);   // текущее состояние — в redo
        pixels = undoStack.pop(); // откат к предыдущему
        update();
    }
}

void PixelCanvas::redo() {
    if (!redoStack.isEmpty()) {
        undoStack.push(pixels);   // текущее состояние — в undo
        pixels = redoStack.pop(); // восстановление
        update();
    }
}






// ---------- алгоритмы с измерением времени ----------
void PixelCanvas::drawLineStep(QPoint a, QPoint b) {
    QElapsedTimer timer;
    timer.start();

    QColor color = algorithmColor(currentAlg);

    int x1 = a.x(), y1 = a.y();
    int x2 = b.x(), y2 = b.y();

    int dx = x2 - x1;
    int dy = y2 - y1;

    // вертикальная линия
    if (dx == 0) {
        int y_min = std::min(y1, y2);
        int y_max = std::max(y1, y2);
        for (int y = y_min; y <= y_max; ++y)
            setPixel(QPoint(x1, y), color);
        timesStep.append(timer.nsecsElapsed() / 1e6);
        return;
    }

    float k = static_cast<float>(dy) / static_cast<float>(dx);

    // пологая линия (шагаем по X)
    if (std::abs(dx) >= std::abs(dy)) {
        if (x1 > x2) { std::swap(x1, x2); std::swap(y1, y2); dx = x2 - x1; dy = y2 - y1; k = static_cast<float>(dy) / dx; }

        for (int x = x1; x <= x2; ++x) {
            int y = static_cast<int>(std::round(y1 + k * (x - x1)));
            setPixel(QPoint(x, y), color);
        }
    }
    // крутая линия (шагаем по Y)
    else {
        if (y1 > y2) { std::swap(x1, x2); std::swap(y1, y2); dx = x2 - x1; dy = y2 - y1; k = static_cast<float>(dy) / dx; }

        float inv_k = static_cast<float>(dx) / static_cast<float>(dy);
        for (int y = y1; y <= y2; ++y) {
            int x = static_cast<int>(std::round(x1 + inv_k * (y - y1)));
            setPixel(QPoint(x, y), color);
        }
    }

    qreal t = timer.nsecsElapsed() / 1e6;
    timesStep.append(t);
    qDebug() << "Step algorithm time:" << t << "ms";
}


void PixelCanvas::drawLineDDA(QPoint a, QPoint b) {
    QElapsedTimer timer;
    timer.start();

    int x1 = a.x();
    int y1 = a.y();
    int x2 = b.x();
    int y2 = b.y();

    int dx = x2 - x1;
    int dy = y2 - y1;

    int L = std::max(std::abs(dx), std::abs(dy));
    if (L == 0) {
        setPixel(a);
        qDebug() << "DDA algorithm time:" << timer.nsecsElapsed() / 1e6 << "ms";
        return;
    }

    float x_inc = dx / static_cast<float>(L);
    float y_inc = dy / static_cast<float>(L);

    float x = x1;
    float y = y1;

    for (int i = 0; i <= L; i++) {
        setPixel(QPoint(std::round(x), std::round(y)), algorithmColor(currentAlg));
        x += x_inc;
        y += y_inc;
    }

    // --- DDA ---
    qreal t = timer.nsecsElapsed() / 1e6;
    timesDDA.append(t);
    qDebug() << "DDA algorithm time:" << t << "ms";
}


void PixelCanvas::drawLineBresenham(QPoint a, QPoint b) {
    QElapsedTimer timer;
    timer.start();

    int x1 = a.x();
    int y1 = a.y();
    int x2 = b.x();
    int y2 = b.y();

    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);

    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;

    bool steep = dy > dx;
    if (steep) std::swap(dx, dy);

    int err = 2 * dy - dx;

    for (int i = 0; i <= dx; i++) {
        setPixel(QPoint(x1, y1), algorithmColor(currentAlg));

        if (err >= 0) {
            if (steep)
                x1 += sx;
            else
                y1 += sy;
            err -= 2 * dx;
        }

        if (steep)
            y1 += sy;
        else
            x1 += sx;

        err += 2 * dy;
    }

    // --- Bresenham (line) ---
    qreal t = timer.nsecsElapsed() / 1e6;
    timesBresenhamLine.append(t);
    qDebug() << "Bresenham (line) time:" << t << "ms";
}


void PixelCanvas::drawCircleBresenham(QPoint center, int radius) {
    QElapsedTimer timer;
    timer.start();

    int x0 = center.x();
    int y0 = center.y();

    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;

    auto drawSymmetricPoints = [&](int x, int y) {
        QColor c = algorithmColor(currentAlg);
        setPixel(QPoint(x0 + x, y0 + y), c);
        setPixel(QPoint(x0 - x, y0 + y), c);
        setPixel(QPoint(x0 + x, y0 - y), c);
        setPixel(QPoint(x0 - x, y0 - y), c);
        setPixel(QPoint(x0 + y, y0 + x), c);
        setPixel(QPoint(x0 - y, y0 + x), c);
        setPixel(QPoint(x0 + y, y0 - x), c);
        setPixel(QPoint(x0 - y, y0 - x), c);
    };

    while (x <= y) {
        drawSymmetricPoints(x, y);

        if (d >= 0) {
            d += 4 * (x - y) + 10;
            y--;
        } else {
            d += 4 * x + 6;
        }

        x++;
    }

    // --- Bresenham (circle) ---
    qreal t = timer.nsecsElapsed() / 1e6;
    timesBresenhamCircle.append(t);
    qDebug() << "Bresenham (circle) time:" << t << "ms";
}
