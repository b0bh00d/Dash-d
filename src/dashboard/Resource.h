#pragma once

#include <QWidget>
#include <QSharedPointer>

#include <QPainter>
#include <QPaintEvent>

#include "Types.h"

// The Resource class represents a value with which a Sensor is concerned.  A
// Resource also has a display presence to show its state.

class Resource : public QWidget
{
    Q_OBJECT

public:    // typedefs and enums
    const static QMap<ResourceState, QString> StateImages;

public:
    explicit Resource(std::uint64_t id, const QString& label, const QString& background_color = "white", QWidget *parent = nullptr);

    void            show_label(bool show = true) const;

    ResourceState   state() const {return m_state; }
    void            set_state(ResourceState state);

    QString         label() const;
    void            set_label(const QString &newLabel);

protected:  // methods
    void    paintEvent(QPaintEvent*) Q_DECL_OVERRIDE;

private:    // typedefs and enums

private:    // methods

private:    // data members
    std::uint64_t   m_id;
    QString         m_label;

    ResourceState   m_state{ResourceState::Undefined};

    QColor          m_background_color;
    int             m_margin{10};

    // QString         m_font;
};

using ResourcePtr = QSharedPointer<Resource>;
