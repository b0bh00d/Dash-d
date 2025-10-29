#include <cassert>

#include <QTimer>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QPlainTextEdit>

#include "Resource.h"

const QMap<ResourceState, QString> Resource::StateImages{
    {ResourceState::Healthy, QStringLiteral(":/images/Healthy.png")},
    {ResourceState::Poor, QStringLiteral(":/images/Poor.png")},
    {ResourceState::Critical, QStringLiteral(":/images/Critical.png")},
};

Resource::Resource(const QString& label, const QString& background_color, QWidget *parent)
    : QWidget{parent},
      m_label(label),
      m_background_color(background_color)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);// | Qt::WindowStaysOnTopHint);

//     auto h1Layout = new QHBoxLayout;
//     h1Layout->setMargin(5);

//     h1Layout->addStretch();

//     auto check = new QCheckBox("");
// //    check->setStyleSheet("* { border-color: blue }");
//     check->setChecked(false);
//     check->setObjectName("check_Delete");
//     connect(check, &QCheckBox::clicked, this, &Harbinger::slot_delete_note);

//     h1Layout->addWidget(check);

//     auto vLayout = new QVBoxLayout;
//     vLayout->setMargin(5);
//     vLayout->addLayout(h1Layout);

// #ifdef QT_WIN
//     auto te = new QTextEdit(m_msg);
// #endif
// #ifdef QT_LINUX
//     auto te = new QTextEdit();
//     QTextCursor cursor(te->textCursor());

//     QTextCharFormat format;
//     //format.setFontWeight(QFont::DemiBold);
//     format.setForeground(QBrush(QColor("black")));
//     cursor.setCharFormat(format);

//     cursor.insertText(m_label);
// #endif
//     QPalette p = te->palette();
//     p.setColor(QPalette::Active, QPalette::Base, QColor(0,0,0,0));
//     p.setColor(QPalette::Inactive, QPalette::Base, QColor(0,0,0,0));
//     te->setPalette(p);
//     te->setFrameStyle(QFrame::NoFrame);
//     te->setWordWrapMode(QTextOption::WordWrap);
//     te->setReadOnly(true);
//     te->setAlignment(Qt::AlignCenter);
//     te->setTextInteractionFlags(te->textInteractionFlags() | Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
//     auto f = this->font();
//     f.fromString(m_font);
//     te->setFont(f);

//     vLayout->addWidget(te);

//     vLayout->addStretch();

//     setLayout(vLayout);
}

void Resource::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    QRect s = geometry();
    QRect r(0, 0, s.width(), s.height());

    painter.fillRect(r, m_background_color);//QBrush(QColor(Qt::yellow).darker(100)));
    r.adjust(m_margin, m_margin, -m_margin, -m_margin);

    painter.save();
//     painter.setOpacity(
// #ifdef QT_WIN
//     .07
// #endif
// #ifdef QT_LINUX
//     .08
// #endif
//     );
    auto image = QImage(StateImages[m_state]).scaledToHeight(r.height());
    painter.drawImage(r, image);
    painter.restore();
}

QString Resource::label() const
{
    return m_label;
}

void Resource::setLabel(const QString &newLabel)
{
    m_label = newLabel;
}

void Resource::show_label(bool show) const
{
    Q_UNUSED(show)
}

void Resource::set_state(ResourceState state)
{
    assert(state != ResourceState::Undefined);

    if(state != m_state)
    {
        bool need_repaint{true};

        // The first call to set_state() will open the Harbinger window
        if(m_state == ResourceState::Undefined)
        {
            // open the window
            this->show();
            need_repaint = false;
        }

        m_state = state;

        if(need_repaint)
            this->repaint();
    }
}
