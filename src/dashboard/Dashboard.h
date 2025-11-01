#pragma once

#include <tuple>

#include <QFrame>
#include <QLabel>

#include <QSharedPointer>

#include <QEvent>
#include <QMouseEvent>

#include <QPainter>
#include <QPaintEvent>

#include <QPropertyAnimation>

#include "Domain.h"
#include "Sensor.h"
#include "Types.h"

// The Dashboard class is a display container for Senors.

class Dashboard : public QFrame
{
    Q_OBJECT
public:    // typedefs and enums
    enum class Orientation {
        Symmetrical,
        Vertical,
        Horizontal
    };

    enum class Direction {
        Left,
        Right,
        Up,
        Down
    };

public:
    explicit Dashboard(bool dark_mode, bool always_on_top,
        Orientation orientation = Orientation::Vertical,
        Direction direction = Direction::Down,
        QFrame *parent = nullptr);

    void        add_sensor(SensorPtr sensor);
    void        del_sensor(SensorPtr sensor);
    void        del_sensor(std::uint64_t id);

signals:
    void        signal_dash_moved(QPoint pos);

public slots:
    void        slot_add_sensor(SensorPtr sensor);
    void        slot_del_sensor(std::uint64_t id);
    void        slot_update_sensor(std::uint64_t id, SensorState state, bool notify);

protected:  // methods
    void        paintEvent(QPaintEvent*) Q_DECL_OVERRIDE;
    void        enterEvent(QEvent* event) Q_DECL_OVERRIDE;
    void        leaveEvent(QEvent* event) Q_DECL_OVERRIDE;
    void        mousePressEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    void        mouseReleaseEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    void        mouseMoveEvent(QMouseEvent* event) Q_DECL_OVERRIDE;

private slots:
    void        slot_flash_notify();

    void        slot_add_sensor_animation_complete();
    void        slot_del_sensor_animation_complete();
    void        slot_animate_del();

private:    // typedefs and enums
    using LabelMap = QMap<std::uint64_t, QLabel*>;

    using ActionStack = std::tuple<Domain*, Sensor*, int, int>;
    using ActionMap = std::map<QPropertyAnimation*, ActionStack>;

private:    // methods
    void        add_sensor(Domain* domain, Sensor* sensor, int w, int h);
    void        del_sensor();

private:    // data members
    int         m_margin{15};
    bool        m_dark_mode{true};

    Orientation m_orientation{Orientation::Symmetrical};
    Direction   m_direction{Direction::Down};

    LabelMap    m_labels;

    // Our starting geometry; we grow from, or shrink back to,
    // this, depending on the specified orientation
    QPoint      m_base_pos;
    QSize       m_base_dim;
    // This is our current size
    QPoint      m_current_pos;
    QSize       m_current_dim;

    int         m_sensor_size{0};  // pixel width or height, depending on window orientation

    // If a state change indicates 'notify', we flash the next
    // state image with an 'empty' version to get the users attention.
    QPixmap     m_next_image;
    QPixmap     m_empty_image;
    QLabel*     m_target_sensor{nullptr};
    bool        m_showing_empty{false};
    int         m_flash_count{0};

    // Variables for supporting direct window moving
    bool        m_left_button{false};   // true if the left button is being pressed
    bool        m_right_button{false};  // true if the left button is being pressed
    bool        m_moving{false};
    int         m_left_offset{0};
    int         m_top_offset{0};
    QPoint      m_start_pos;

    ActionMap   m_actions;
};

using DashboardPtr = QSharedPointer<Dashboard>;
