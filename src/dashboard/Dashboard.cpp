#include <QDebug>

#include <QUrl>
#include <QTimer>

#include <QLabel>
#include <QBitmap>
#include <QPainterPath>

#include <QHBoxLayout>
#include <QVBoxLayout>

#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

#include "Domain.h"
#include "Dashboard.h"

Dashboard::Dashboard(bool dark_mode, bool always_on_top, Orientation orientation, Direction direction, QFrame *parent)
    : QFrame{parent},
      m_dark_mode(dark_mode),
      m_orientation(orientation),
      m_direction(direction)
{
    auto flags = Qt::Tool | Qt::FramelessWindowHint;
    if(always_on_top)
        flags |= Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);

    QBoxLayout* layout{nullptr};
    QSpacerItem* spacer{nullptr};
    if(orientation == Orientation::Vertical)
    {
        layout = new QVBoxLayout;
        spacer = new QSpacerItem(20, 16777215, QSizePolicy::Expanding, QSizePolicy::Preferred);
    }
    else
    {
        layout = new QHBoxLayout;
        spacer = new QSpacerItem(16777215, 20, QSizePolicy::Expanding, QSizePolicy::Preferred);
    }
    layout->setMargin(m_margin);
    layout->addSpacerItem(spacer);
    setLayout(layout);
}

// https://forum.qt.io/topic/162168/how-to-round-the-corners-of-a-frameless-qwidget-when-also-applying-acrylic-effects/8
void Dashboard::paintEvent(QPaintEvent* /*event*/)
{
    // Keep the endcaps "pill" shaped regardless of the width/height
    qreal radius = (m_orientation == Orientation::Vertical) ? m_base_dim.width() * 0.75 : m_base_dim.height() * 0.75;

    // Rounded mask
    QBitmap bitmap(width(), height());
    bitmap.fill(Qt::color0);
    QPainter maskPainter(&bitmap);
    maskPainter.setRenderHints(QPainter::Antialiasing);
    QPainterPath maskPath;
    maskPath.addRoundedRect(rect(), radius, radius);
    maskPainter.fillPath(maskPath, Qt::color1);
    setMask(bitmap);

    // Colors
    QColor BG = m_dark_mode ? QColor(0x2D, 0x2D, 0x2D) : QColor(0xFF, 0xFF, 0xFF);
    QColor BR = m_dark_mode ? QColor(0x4D, 0x4D, 0x4D) : QColor(0xBD, 0xBD, 0xBD);

    if(m_mouse_inside)
        BR = m_dark_mode ? BR.lighter() : BR.darker();

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing);
    painter.setBrush(BG);
    QPen pen(BR);
    pen.setWidth(1);
    painter.setPen(pen);

    QPainterPath path;
    path.addRoundedRect(rect().adjusted(1, 1, -1, -1), radius, radius);
    painter.drawPath(path);
}

void Dashboard::enterEvent(QEvent* event)
{
    m_mouse_inside = true;
    repaint();
    QFrame::enterEvent(event);
}

void Dashboard::leaveEvent(QEvent* event)
{
    m_mouse_inside = false;
    repaint();
    QFrame::leaveEvent(event);
}

void Dashboard::mousePressEvent(QMouseEvent* event)
{
    m_left_button = event->button() == Qt::MouseButton::LeftButton;
    m_right_button = event->button() == Qt::MouseButton::RightButton;

    if(m_left_button)
    {
        // Prepare settings for a move
        auto pos = event->globalPos();

        m_start_pos = QPoint(m_current_pos.x(), m_current_pos.y());

        auto left = m_current_pos.x();
        auto top = m_current_pos.y();

        m_left_offset = pos.x() - left;
        m_top_offset = pos.y() - top;
    }

    QFrame::mousePressEvent(event);
}

void Dashboard::mouseReleaseEvent(QMouseEvent* event)
{
    m_left_button = m_right_button = false;

    const auto pos = event->globalPos();

    // TODO: The window's (x,y) position will be wrong in Up/Left situations when there's more than one sensor in the dashboard.
    m_base_pos = QPoint(pos.x() - m_left_offset, pos.y() - m_top_offset);
    m_current_pos = m_base_pos;

    emit signal_dash_moved(m_base_pos);

    QFrame::mouseReleaseEvent(event);
}

void Dashboard::mouseMoveEvent(QMouseEvent* event)
{
    if(!m_moving)
    {
        if(m_left_button)
        {
            // We are now moving
            m_moving = true;
        }
    }
    else    // We are curently moving
    {
        if(!m_left_button)
            // Not anymore
            m_moving = false;
        else
        {
            const auto pos = event->globalPos();

            auto left = pos.x() - m_left_offset;
            auto top = pos.y() - m_top_offset;

            move(left, top);
        }
    }

    QFrame::mouseMoveEvent(event);
}

QString Dashboard::gen_tooltip(const QString& base, const QString& msg)
{
    auto tt1 = tr("<code>%1</code>").arg(base);
    QString tt2;
    if(!msg.isEmpty())
        tt2 = tr("Event: %1").arg(QUrl::fromPercentEncoding(msg.toUtf8()));
    auto tt3 = tr("Updated: %1").arg(QDateTime::currentDateTime().toString());

    auto tooltip = QString("%1<hr>%2%3")
        .arg(tt1, tt2.isEmpty() ? "" : QString("%1<br>").arg(tt2), tt3);
    return(tooltip);
}

void Dashboard::add_sensor(SensorPtr sensor)
{
    auto domain = qobject_cast<Domain*>(sender());

    if(m_base_pos.isNull())
    {
        // Perform first-sensor calculations.
        auto g = geometry();

        m_base_pos = QPoint(g.left(), g.top());
        m_base_dim = QSize(g.width(), g.height());

        m_current_pos = m_base_pos;
        m_current_dim = m_base_dim;

        QString image = QStringLiteral(":/images/Empty.png");
        m_empty_image = (m_orientation == Orientation::Vertical) ? QPixmap(image).scaledToWidth(m_base_dim.width()) : QPixmap(image).scaledToHeight(m_base_dim.height());
        m_sensor_size = (m_orientation == Orientation::Vertical) ? m_empty_image.height() : m_empty_image.width();
    }

    const int w = (m_orientation == Orientation::Vertical) ? m_current_dim.width() : (m_sensor_size + m_margin) * (m_labels.count() + 1);
    const int h = (m_orientation == Orientation::Vertical) ? (m_sensor_size + m_margin) * (m_labels.count() + 1) : m_current_dim.height();

    const int w_delta = abs(w - m_current_dim.width());
    const int h_delta = abs(h - m_current_dim.height());

    // Resize the window to accomdate the new Sensor display (if required)
    if(m_labels.count())
    {
        // Animate the movement/resizing of the window

        QPropertyAnimation* animation{nullptr};
        if(m_orientation == Orientation::Vertical)
        {
            if(m_direction == Direction::Up)
            {
                // We are moving the window's origin and resizing at the same time

                animation = new QPropertyAnimation(this, "geometry");
                animation->setStartValue(QRect(m_current_pos.x(), m_current_pos.y(), m_current_dim.width(), m_current_dim.height()));
                animation->setEndValue(QRect(m_current_pos.x(), m_current_pos.y() - h_delta, m_current_dim.width(), h));

                m_current_pos.setY(m_current_pos.y() - h_delta);
            }
            else if(m_direction == Direction::Down)
            {
                // We are only resizing the window

                animation = new QPropertyAnimation(this, "geometry");
                animation->setStartValue(QRect(m_current_pos.x(), m_current_pos.y(), m_current_dim.width(), m_current_dim.height()));
                animation->setEndValue(QRect(m_current_pos.x(), m_current_pos.y(), m_current_dim.width(), h));
            }
            else
                assert(false);
        }
        else if(m_orientation == Orientation::Horizontal)
        {
            if(m_direction == Direction::Left)
            {
                // We are moving the window's origin and resizing at the same time

                animation = new QPropertyAnimation(this, "geometry");
                animation->setStartValue(QRect(m_current_pos.x(), m_current_pos.y(), m_current_dim.width(), m_current_dim.height()));
                animation->setEndValue(QRect(m_current_pos.x() - w_delta, m_current_pos.y(), w, m_current_dim.height()));

                m_current_pos.setX(m_current_pos.x() - w_delta);
            }
            else if(m_direction == Direction::Right)
            {
                // We are only resizing the window
                animation = new QPropertyAnimation(this, "geometry");
                animation->setStartValue(QRect(m_current_pos.x(), m_current_pos.y(), m_current_dim.width(), m_current_dim.height()));
                animation->setEndValue(QRect(m_current_pos.x(), m_current_pos.y(), w, m_current_dim.height()));
            }
            else
                assert(false);
        }

        if(animation)
        {
            ActionStack items{domain, sensor.data(), w, h};
            m_actions[animation] = items;

            animation->setDuration(500);
            animation->setEasingCurve(QEasingCurve::OutSine);
            connect(animation, &QAbstractAnimation::finished, this, &Dashboard::slot_add_sensor_animation_complete);
            animation->start();
        }
        else
            assert(false);
    }
    else
        add_sensor(domain, sensor.data(), w, h);
}

void Dashboard::add_sensor(Domain* domain, Sensor* sensor, int w, int h)
{
    auto tooltip = QString("%1::%2").arg(domain->name(), sensor->name());

    auto image = Sensor::StateImages[sensor->state()];
    QPixmap pixmap = (m_orientation == Orientation::Vertical) ? QPixmap(image).scaledToWidth(m_base_dim.width()) : QPixmap(image).scaledToHeight(m_base_dim.height());

    auto label = new QLabel();
    label->setPixmap(pixmap);

    label->setProperty("base_tooltip", tooltip);
    tooltip = gen_tooltip(label->property("base_tooltip").toString(), sensor->message());
    label->setToolTip(tooltip);

    QBoxLayout* my_layout = reinterpret_cast<QBoxLayout*>(layout());
    const int count = my_layout->count();

    switch(m_direction)
    {
        case Direction::Up:
            if(count == 1)
                my_layout->addWidget(label);
            else
                my_layout->insertWidget(my_layout->count() - 1, label);
            break;
        case Direction::Left:
            if(count == 1)
                my_layout->addWidget(label);
            else
                my_layout->insertWidget(my_layout->count() - 1, label);
            break;
        case Direction::Down:
            if(count == 1)
                my_layout->insertWidget(0, label);
            else
                my_layout->insertWidget(my_layout->count() - 1, label);
            break;
        case Direction::Right:
            if(count == 1)
                my_layout->insertWidget(0, label);
            else
                my_layout->insertWidget(my_layout->count() - 1, label);
            break;

        default:
            assert(false);
    }

    if(m_labels.isEmpty())
        show();

    m_labels[sensor->name()] = label;

    m_current_dim = QSize(w, h);

    repaint();
}

void Dashboard::del_sensor(SensorPtr sensor)
{
    del_sensor(sensor->name());
}

void Dashboard::del_sensor(const QString& name)
{
    auto label = m_labels[name];
    m_labels.remove(name);

    auto my_layout = reinterpret_cast<QVBoxLayout*>(layout());
    my_layout->takeAt(my_layout->indexOf(label));
    label->deleteLater();

    if(m_labels.isEmpty())
    {
        hide();
        m_current_pos = m_base_pos;
        m_current_dim = m_base_dim;
    }
    else
        QTimer::singleShot(0, this, &Dashboard::slot_animate_del);
}

void Dashboard::slot_add_sensor(SensorPtr sensor)
{
    add_sensor(sensor);
}

void Dashboard::slot_del_sensor(const QString& name)
{
    del_sensor(name);
}

void Dashboard::slot_update_sensor(const QString& name, SharedTypes::SensorState state, const QString& message, bool notify)
{
    m_target_sensor = m_labels[name];

    QString image = Sensor::StateImages[state];
    m_next_image = (m_orientation == Orientation::Vertical) ? QPixmap(image).scaledToWidth(m_base_dim.width()) : QPixmap(image).scaledToHeight(m_base_dim.height());

    if(notify)
    {
        m_target_sensor->setPixmap(m_empty_image);
        m_showing_empty = true;
        m_flash_count = 10;
        QTimer::singleShot(100, this, &Dashboard::slot_flash_notify);
    }
    else
        m_target_sensor->setPixmap(m_next_image);

    auto tooltip = gen_tooltip(m_target_sensor->property("base_tooltip").toString(), message);
    m_target_sensor->setToolTip(tooltip);
}

void Dashboard::slot_flash_notify()
{
    m_flash_count -= 1;
    if(!m_flash_count)
    {
        // We're done
        m_target_sensor->setPixmap(m_next_image);
    }
    else
    {
        m_target_sensor->setPixmap(m_showing_empty ? m_next_image: m_empty_image);
        m_showing_empty = !m_showing_empty;
        QTimer::singleShot(100, this, &Dashboard::slot_flash_notify);
    }
}

void Dashboard::slot_add_sensor_animation_complete()
{
    auto animation = qobject_cast<QPropertyAnimation*>(sender());
    animation->deleteLater();

    auto tuple = m_actions[animation];
    m_actions.erase(animation);

    auto domain = std::get<0>(tuple);
    auto sensor = std::get<1>(tuple);
    auto w = std::get<2>(tuple);
    auto h = std::get<3>(tuple);

    add_sensor(domain, sensor, w, h);
}

void Dashboard::slot_animate_del()
{
    // Deleting a sensor is done a in reverse from adding because we want to visually
    // remove the avatar from the window first, and THEN animate the window collapsing.

    const int w = (m_orientation == Orientation::Vertical) ? m_current_dim.width() : m_sensor_size * m_labels.count();
    const int h = (m_orientation == Orientation::Vertical) ? m_sensor_size * m_labels.count() : m_current_dim.height();

    const int w_delta = abs(w - m_current_dim.width());
    const int h_delta = abs(h - m_current_dim.height());

    // Animate the movement/resizing of the window

    QPropertyAnimation* animation{nullptr};
    if(m_orientation == Orientation::Vertical)
    {
        if(m_direction == Direction::Up)
        {
            // We are moving the window's origin and resizing at the same time

            animation = new QPropertyAnimation(this, "geometry");
            animation->setStartValue(QRect(m_current_pos.x(), m_current_pos.y(), m_current_dim.width(), m_current_dim.height()));
            animation->setEndValue(QRect(m_current_pos.x(), m_current_pos.y() + h_delta, m_current_dim.width(), h));

            m_current_pos.setY(m_current_pos.y() - h_delta);
        }
        else if(m_direction == Direction::Down)
        {
            // We are only resizing the window

            animation = new QPropertyAnimation(this, "geometry");
            animation->setStartValue(QRect(m_current_pos.x(), m_current_pos.y(), m_current_dim.width(), m_current_dim.height()));
            animation->setEndValue(QRect(m_current_pos.x(), m_current_pos.y(), m_current_dim.width(), h));
        }
        else
            assert(false);
    }
    else if(m_orientation == Orientation::Horizontal)
    {
        if(m_direction == Direction::Left)
        {
            // We are moving the window's origin and resizing at the same time

            animation = new QPropertyAnimation(this, "geometry");
            animation->setStartValue(QRect(m_current_pos.x(), m_current_pos.y(), m_current_dim.width(), m_current_dim.height()));
            animation->setEndValue(QRect(m_current_pos.x() + w_delta, m_current_pos.y(), w, m_current_dim.height()));

            m_current_pos.setX(m_current_pos.x() - w_delta);
        }
        else if(m_direction == Direction::Right)
        {
            // We are only resizing the window
            animation = new QPropertyAnimation(this, "geometry");
            animation->setStartValue(QRect(m_current_pos.x(), m_current_pos.y(), m_current_dim.width(), m_current_dim.height()));
            animation->setEndValue(QRect(m_current_pos.x(), m_current_pos.y(), w, m_current_dim.height()));
        }
        else
            assert(false);
    }

    if(animation)
    {
        animation->setDuration(500);
        animation->setEasingCurve(QEasingCurve::OutSine);
        connect(animation, &QAbstractAnimation::finished, this, &Dashboard::slot_del_sensor_animation_complete);
        animation->start();
    }
    else
        assert(false);
}

void Dashboard::slot_del_sensor_animation_complete()
{
    auto animation = qobject_cast<QPropertyAnimation*>(sender());
    animation->deleteLater();
}
