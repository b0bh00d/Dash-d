#pragma once

#include <QMap>
#include <QList>
#include <QMenu>
#include <QAction>
#include <QDialog>
#include <QByteArray>
#include <QMessageBox>
#include <QShowEvent>
#include <QCloseEvent>
#include <QSystemTrayIcon>

#include "Dashboard.h"
#include "Domain.h"
#include "Sender.h"
#include "Receiver.h"

// This is the initial width/height of the dashboard window.
constexpr int base_symmetry{75};

QT_BEGIN_NAMESPACE
namespace Ui {
class Dialog;
}
QT_END_NAMESPACE

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);
    ~Dialog();

protected: // methods
    void        closeEvent(QCloseEvent *event);

private slots:
    void        slot_set_control_states();

    void        slot_quit();

    void        slot_tray_icon_activated(QSystemTrayIcon::ActivationReason reason);
    void        slot_tray_message_clicked();
    void        slot_tray_menu_action(QAction* action);

    void        slot_process_peer_event(const QByteArray& datagram);

    void        slot_randomize_ipv4();
    void        slot_randomize_ipv6();

    void        slot_multicast_group_join();

    void        slot_accept_settings();
    void        slot_reject_settings();

    void        slot_orientation_vertical();
    void        slot_orientation_horizontal();
    void        slot_direction_downright();
    void        slot_direction_upleft();

    void        slot_dash_moved(QPoint pos);

#ifdef TEST
    void        slot_test_insert_sensor();
    void        slot_test_poor_sensor();
    void        slot_test_critical_sensor();
    void        slot_test_remove_sensor();
#endif

private:    // typedefs and emums
    using DomainMap = QMap<uint64_t, DomainPtr>;

private:    // methods
    void        build_tray_menu();

    void        load_settings();
    void        save_settings();

private:    // data members
    Ui::Dialog* ui;

    DashboardPtr m_dashboard;

    QSystemTrayIcon* m_trayIcon{nullptr};
    QMenu*      m_trayIconMenu{nullptr};
    QAction*    m_quit_action{nullptr};

    SenderPtr   m_multicast_sender;
    ReceiverPtr m_multicast_receiver;

    bool        m_multicast_group_member{false};
    bool        m_randomized_addresses{false};

    // List of domains we've heard from
    DomainMap   m_domains;

    QPoint      m_dash_pos;

    Dashboard::Orientation  m_orientation{Dashboard::Orientation::Vertical};
    Dashboard::Direction    m_direction{Dashboard::Direction::Down};

    QString     m_version;

#ifdef TEST
    int         m_test_count{0};
#endif
};
