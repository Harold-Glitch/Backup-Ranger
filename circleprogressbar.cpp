
// circleprogressbar.cpp
#include "circleprogressbar.h"
#include <QPainter>
#include <QDebug>
#include <QtMath>

CircleProgressBar::CircleProgressBar(QWidget *parent)
    : QWidget(parent),
    m_value(0),
    m_minimum(0),
    m_maximum(100),
    m_progressColor(Qt::green),
    m_backgroundColor(Qt::gray) {
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

void CircleProgressBar::setValue(int value) {
    if (value < m_minimum || value > m_maximum || value == m_value) {
        return;
    }
    m_value = value;
    update(); // Trigger repaint
    emit valueChanged(value);
}

void CircleProgressBar::setMinimum(int min) {
    if (min >= m_maximum) return;
    m_minimum = min;
    if (m_value < min) setValue(min);
    update();
}

void CircleProgressBar::setMaximum(int max) {
    if (max <= m_minimum) return;
    m_maximum = max;
    if (m_value > max) setValue(max);
    update();
}

void CircleProgressBar::setRange(int min, int max) {
    if (min >= max) return;
    m_minimum = min;
    m_maximum = max;
    if (m_value < min) setValue(min);
    else if (m_value > max) setValue(max);
    update();
}

void CircleProgressBar::setProgressColor(const QColor &color) {
    m_progressColor = color;
    update();
}

void CircleProgressBar::setTextColor(const QColor &color) {
    m_textColor = color;
    update();
}

void CircleProgressBar::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int side = qMin(width(), height());
    int thickness = side / 10; // Thickness of the circle
    QRectF rect((width() - side) / 2, (height() - side) / 2, side, side);
    rect.adjust(thickness / 2, thickness / 2, -thickness / 2, -thickness / 2);

    // Draw background circle
    painter.setPen(QPen(m_backgroundColor, thickness, Qt::SolidLine, Qt::FlatCap));
    painter.drawEllipse(rect);

    // Calculate progress angle (0-360 degrees)
    qreal progress = static_cast<qreal>(m_value - m_minimum) / (m_maximum - m_minimum);
    int angle = qRound(progress * 360);

    // Draw progress arc
    painter.setPen(QPen(m_progressColor, thickness, Qt::SolidLine, Qt::FlatCap));
    painter.drawArc(rect, 90 * 16, -angle * 16); // Start at 12 o'clock, clockwise

    // Optional: Draw percentage text
    painter.setPen(m_textColor);  // m_backgroundColor
    painter.setFont(QFont("Arial", side / 7));
    painter.drawText(rect, Qt::AlignCenter, QString("%1%").arg(qRound(progress * 100)));
}
