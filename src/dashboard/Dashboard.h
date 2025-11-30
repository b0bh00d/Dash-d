#pragma once

#include <tuple>

#include <QSharedPointer>
#include <QQueue>

#include <QFrame>
#include <QLabel>

#include <QEvent>
#include <QMouseEvent>

#include <QPainter>
#include <QPaintEvent>

#include <QPropertyAnimation>

// #include "../SharedTypes.h"

#include "Domain.h"
#include "Sensor.h"

//---------------------------------------------------------------------------
// Dashboard
//
// The Dashboard is a visual display of the health of an asset or resource as
// reported by a Sensor in a Domain.
//
//---------------------------------------------------------------------------

class Dashboard : public QFrame
{
    Q_OBJECT
public:    // typedefs and enums
    // Should the dashboard be North/South or East/West?
    enum class Orientation {
        Symmetrical,
        Vertical,
        Horizontal
    };

    // Which direction should the dashboard expand when new Sensors are added?
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

signals:
    void        signal_dash_moved(QPoint pos);

public slots:
    void        slot_add_sensor(SensorPtr sensor, Domain* domain = nullptr);
    void        slot_del_sensor(SensorPtr sensor);
    void        slot_update_sensor(SensorPtr sensor, const QString& message, bool notify);

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
    void        animate_sensor_add(SensorPtr sensor, Domain* domain = nullptr);
    void        slot_del_sensor_animation_complete();
    void        animate_del_sensor(SensorPtr sensor);
    void        slot_animate_del();

    void        slot_housekeeping();

private:    // typedefs and enums
    enum class SensorAction { None, Add, Delete, Update };

    using LabelMap = QMap<QString, QLabel*>;

    using AnimData = std::tuple<Domain*, Sensor*, int, int>;
    using AnimMap = std::map<QPropertyAnimation*, AnimData>;

    using ActionData = std::tuple<Domain*, SensorPtr, SensorAction>;
    using ActionQueue = QQueue<ActionData>;

    using TimerPtr = QSharedPointer<QTimer>;

private:    // methods
    void        add_sensor(Domain* domain, Sensor* sensor, int w, int h);
    void        del_sensor();
    QString     gen_tooltip(Sensor* sensor, const QString& base, const QString& msg = QString());

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
    bool        m_right_button{false};  // true if the right button is being pressed
    bool        m_moving{false};
    int         m_left_offset{0};
    int         m_top_offset{0};
    QPoint      m_start_pos;

    AnimMap     m_anim_data;
    ActionQueue m_action_queue;

    bool        m_mouse_inside{false};
    bool        m_animation_in_progress{false};

    TimerPtr    m_housekeeping;
};

using DashboardPtr = QSharedPointer<Dashboard>;
