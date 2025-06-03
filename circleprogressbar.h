// circleprogressbar.h
#ifndef CIRCLEPROGRESSBAR_H
#define CIRCLEPROGRESSBAR_H

#include <QWidget>

class CircleProgressBar : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(int maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(QColor progressColor READ progressColor WRITE setProgressColor)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor)

public:
    explicit CircleProgressBar(QWidget *parent = nullptr);

    int value() const { return m_value; }
    int minimum() const { return m_minimum; }
    int maximum() const { return m_maximum; }
    QColor progressColor() const { return m_progressColor; }
    QColor textColor() const { return m_textColor; }

public slots:
    void setValue(int value);
    void setMinimum(int min);
    void setMaximum(int max);
    void setProgressColor(const QColor &color);
    void setTextColor(const QColor &color);
    void setRange(int min, int max);

signals:
    void valueChanged(int value);

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override { return QSize(100, 100); }
    QSize minimumSizeHint() const override { return QSize(50, 50); }

private:
    int m_value;
    int m_minimum;
    int m_maximum;
    QColor m_progressColor;
    QColor m_backgroundColor;
    QColor m_textColor;
};

#endif // CIRCLEPROGRESSBAR_H
