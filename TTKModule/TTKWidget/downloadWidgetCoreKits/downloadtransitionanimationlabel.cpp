#include "downloadtransitionanimationlabel.h"

#include <QPainter>
#include <QPropertyAnimation>

DownloadTransitionAnimationLabel::DownloadTransitionAnimationLabel(QWidget *parent)
    : QLabel(parent)
{
    m_isAnimating = false;
    m_currentValue = 0;
    m_noAnimationSet = false;

    m_animation = new QPropertyAnimation(this, QByteArray());
    m_animation->setDuration(200);
    m_animation->setEasingCurve(QEasingCurve::Linear);
    m_animation->setStartValue(0);
    m_animation->setEndValue(101);
    connect(m_animation, SIGNAL(valueChanged(QVariant)), SLOT(valueChanged(QVariant)));
    connect(m_animation, SIGNAL(finished()), SLOT(animationFinished()));
}

DownloadTransitionAnimationLabel::~DownloadTransitionAnimationLabel()
{
    delete m_animation;
}

QString DownloadTransitionAnimationLabel::getClassName()
{
    return staticMetaObject.className();
}

QPixmap DownloadTransitionAnimationLabel::getRendererPixmap() const
{
    return m_rendererPixmap;
}

void DownloadTransitionAnimationLabel::stop()
{
    if(m_animation->state() == QPropertyAnimation::Running)
    {
        m_animation->stop();
    }
}

void DownloadTransitionAnimationLabel::setPixmap(const QPixmap &pix)
{
//    if(m_isAnimating)
//    {
//        return;
//    }

    if(m_noAnimationSet || !pixmap())
    {
        m_rendererPixmap = pix;
        QLabel::setPixmap(pix);
        return;
    }

    m_previousPixmap = *pixmap();
    m_currentPixmap = pix;
    m_isAnimating = true;
    m_animation->start();
}

void DownloadTransitionAnimationLabel::valueChanged(const QVariant &value)
{
    m_currentValue = value.toFloat();
    update();
}

void DownloadTransitionAnimationLabel::animationFinished()
{
    m_currentValue = 0;
    m_isAnimating = false;
    m_rendererPixmap = m_currentPixmap;

    QLabel::setPixmap(m_rendererPixmap);
}

void DownloadTransitionAnimationLabel::paintEvent(QPaintEvent *event)
{
    if(m_isAnimating)
    {
        QPainter painter(this);
        painter.drawPixmap(rect(), m_previousPixmap);

        QPixmap pixed( size() );
        pixed.fill(Qt::transparent);
        QPainter paint(&pixed);
        paint.fillRect(rect(), QColor(0xFF, 0xFF, 0xFF, 2.55*m_currentValue));
        paint.setCompositionMode(QPainter::CompositionMode_SourceIn);
        paint.drawPixmap(rect(), m_currentPixmap);
        paint.end();

        m_rendererPixmap = pixed;
        painter.drawPixmap(rect(), pixed);
    }
    else
    {
        QLabel::paintEvent(event);
    }
}
