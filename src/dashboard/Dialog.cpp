#include <random>
#include <climits>

#include <QDir>
#include <QUrl>
#include <QTimer>
#include <QMenuBar>
#include <QDateTime>
#include <QSettings>
#include <QStandardPaths>

#include <QJsonDocument>
#include <QJsonObject>

#include "Dialog.h"
#include "ui_dialog.h"

#include "../SharedTypes.h"

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    // std::random_device rd;
    // std::mt19937 rd_mt(rd());
    // std::uniform_int_distribution<unsigned long long> r(
    //         std::numeric_limits<std::uint64_t>::min(),
    //         std::numeric_limits<std::uint64_t>::max()
    //     );

    // auto map = QMap<unsigned long long, bool>();
    // for(int i = 0;i < 10000000;++i)
    // {
    //     auto i1{r(rd_mt)};
    //     if(map.contains(i1))
    //         break;
    //     map[i1] = true;
    // }

    ui->setupUi(this);

    setWindowTitle(tr("Dash'd by Bob Hood"));
    setWindowIcon(QIcon(":/images/Tray.png"));

#ifdef QT_LINUX
    ui->check_AutoStart->setEnabled(false);
#endif

    // insert a menu bar
    // auto myMenuBar = new QMenuBar(this);
    // auto fileMenu = myMenuBar->addMenu("File");

    // auto quitAction = new QAction(QIcon(":/images/Quit.png"), tr("&Quit"), this);
    // fileMenu->addAction(quitAction);
    // connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    // ui->verticalLayout->insertWidget(0, myMenuBar);

    m_quit_action = new QAction(QIcon(":/images/Quit.png"), tr("&Quit"), this);
    connect(m_quit_action, &QAction::triggered, this, &Dialog::slot_quit);

    connect(ui->check_Channels_IPv4, &QCheckBox::clicked, this, &Dialog::slot_set_control_states);
    connect(ui->button_MulticastGroupIPv4_Randomize, &QPushButton::clicked, this, &Dialog::slot_randomize_ipv4);
    connect(ui->check_Channels_IPv6, &QCheckBox::clicked, this, &Dialog::slot_set_control_states);
    connect(ui->button_MulticastGroupIPv6_Randomize, &QPushButton::clicked, this, &Dialog::slot_randomize_ipv6);

    connect(ui->button_Channels_Join, &QPushButton::clicked, this, &Dialog::slot_multicast_group_join);
    connect(ui->check_Channels_AutoRejoin, &QCheckBox::clicked, this, &Dialog::slot_set_control_states);

    connect(ui->radio_Orientation_Vertical, &QRadioButton::clicked, this, &Dialog::slot_orientation_vertical);
    connect(ui->radio_Orientation_Horizontal, &QRadioButton::clicked, this, &Dialog::slot_orientation_horizontal);
    connect(ui->radio_Direction_UpLeft, &QRadioButton::clicked, this, &Dialog::slot_direction_upleft);
    connect(ui->radio_Direction_DownRight, &QRadioButton::clicked, this, &Dialog::slot_direction_downright);

    // QPushButton* cancel_button = ui->buttonBox->button(QDialogButtonBox::Cancel);
    // cancel_button->setVisible(false);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &Dialog::slot_accept_settings);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &Dialog::slot_accept_settings);

    load_settings();

    // connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &Dialog::slot_update_settings);
    // connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &Dialog::slot_dismiss_settings);

    m_trayIcon = new QSystemTrayIcon(this);
    connect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, &Dialog::slot_tray_message_clicked);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &Dialog::slot_tray_icon_activated);

    m_trayIcon->setIcon(QIcon(":/images/Tray.png"));
    m_trayIcon->setToolTip(tr("Dash'd"));
    build_tray_menu();

    m_trayIcon->show();

    QTimer::singleShot(0, this, &Dialog::slot_set_control_states);
}

Dialog::~Dialog()
{
    delete ui;
}

#ifdef TEST
void Dialog::slot_test_insert_sensor()
{
    if(m_domains.isEmpty())
    {
        auto domain = DomainPtr(new Domain(123456789, "corrin"));
        connect(domain.data(), &Domain::signal_sensor_added, m_dashboard.data(), &Dashboard::slot_add_sensor);
        connect(domain.data(), &Domain::signal_sensor_removed, m_dashboard.data(), &Dashboard::slot_del_sensor);
        connect(domain.data(), &Domain::signal_sensor_updated, m_dashboard.data(), &Dashboard::slot_update_sensor);

        m_domains[domain->id()] = domain;
    }

    auto domain = m_domains[123456789];
    auto sensor = SensorPtr(new Sensor(m_test_count, "brix_reactor_monitor"));
    sensor->set_state(SensorState::Healthy);
    domain->add_sensor(sensor);

    QTimer::singleShot(5000, this, &Dialog::slot_test_poor_sensor);
}

void Dialog::slot_test_poor_sensor()
{
    m_domains[123456789]->update_sensor(m_test_count, SensorState::Poor);
    QTimer::singleShot(5000, this, &Dialog::slot_test_critical_sensor);
}

void Dialog::slot_test_critical_sensor()
{
    m_domains[123456789]->update_sensor(m_test_count, SensorState::Critical);

    if(++m_test_count == 2)
        QTimer::singleShot(5000, this, &Dialog::slot_test_remove_sensor);
    else
        QTimer::singleShot(5000, this, &Dialog::slot_test_insert_sensor);
}

void Dialog::slot_test_remove_sensor()
{
    m_domains[123456789]->del_sensor(m_test_count-1);
}
#endif

void Dialog::closeEvent(QCloseEvent* event)
{
    if(m_trayIcon->isVisible())
    {
        hide();
        event->ignore();
    }
}

void Dialog::build_tray_menu()
{
    if (m_trayIconMenu)
    {
        // disconnect(m_trayIconMenu, &QMenu::triggered, this, &Dialog::slot_tray_menu_action);
        delete m_trayIconMenu;
    }

    m_trayIconMenu = new QMenu(this);

    //trayIconMenu->addSeparator();

    // m_trayIconMenu->addAction(m_restore_action);
    m_trayIconMenu->addAction(m_quit_action);

    m_trayIcon->setContextMenu(m_trayIconMenu);

    connect(m_trayIconMenu, &QMenu::triggered, this, &Dialog::slot_tray_menu_action);
}

void Dialog::load_settings()
{
    m_multicast_group_member = false;

    QString settings_file_name;
#ifdef QT_WIN
    settings_file_name = QDir::toNativeSeparators(QString("%1/Dash-d/Settings.ini").arg(qgetenv("APPDATA").constData()));
#endif
#ifdef QT_LINUX
    auto config_dir = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);
    settings_file_name = QDir::toNativeSeparators(QString("%1/Dash-d.ini").arg(config_dir[0]));
#endif
    QSettings settings(settings_file_name, QSettings::IniFormat);

    settings.beginGroup("GUI");
        ui->check_AutoStart->setChecked(settings.value("startup_enabled", false).toBool());

        ui->line_MulticastGroupPort->setText(settings.value("group_port", "").toString());

        ui->check_Channels_IPv4->setChecked(settings.value("ipv4_multicast_group_enabled", false).toBool());
        ui->line_MulticastGroupIPv4->setText(settings.value("ipv4_multicast_group_address", "").toString());

        ui->check_Channels_IPv6->setChecked(settings.value("ipv6_multicast_group_enabled", false).toBool());
        ui->line_MulticastGroupIPv6->setText(settings.value("ipv6_multicast_group_address", "").toString());

        ui->check_Channels_AutoRejoin->setChecked(settings.value("channels_autorejoin", false).toBool());

        m_orientation = settings.value("orientation_vertical", true).toBool() ?
            Dashboard::Orientation::Vertical :
            Dashboard::Orientation::Horizontal;
        m_direction = settings.value("direction_downright", true).toBool() ?
            Dashboard::Direction::Down :
            Dashboard::Direction::Up;

        ui->radio_Orientation_Vertical->setChecked(m_orientation == Dashboard::Orientation::Vertical);
        ui->radio_Orientation_Horizontal->setChecked(m_orientation == Dashboard::Orientation::Horizontal);

        ui->radio_Direction_DownRight->setChecked(m_direction == Dashboard::Direction::Down || m_direction == Dashboard::Direction::Right);
        ui->radio_Direction_UpLeft->setChecked(m_direction == Dashboard::Direction::Up|| m_direction == Dashboard::Direction::Left);

        ui->check_Always_On_Top->setChecked(settings.value("always_on_top", false).toBool());

        m_dash_pos = settings.value("dash_pos", QPoint(100, 100)).toPoint();
    settings.endGroup();

    if (!ui->check_Channels_AutoRejoin->isChecked())
        // Open the main window to remind them that they need
        // to initiate the connection manually
        showNormal();
    else
        QTimer::singleShot(0, this, &Dialog::slot_multicast_group_join);
}

void Dialog::save_settings()
{
    QString settings_file_name;
#ifdef QT_WIN
    settings_file_name = QDir::toNativeSeparators(QString("%1/Dash-d/Settings.ini").arg(qgetenv("APPDATA").constData()));
#endif
#ifdef QT_LINUX
    auto config_dir = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);
    settings_file_name = QDir::toNativeSeparators(QString("%1/Dash-d.ini").arg(config_dir[0]));
#endif
    QSettings settings(settings_file_name, QSettings::IniFormat);

    settings.clear();

    settings.beginGroup("GUI");
        settings.setValue("startup_enabled", ui->check_AutoStart->isChecked());

        settings.setValue("group_port", ui->line_MulticastGroupPort->text());

        settings.setValue("ipv4_multicast_group_enabled", ui->check_Channels_IPv4->isChecked());
        settings.setValue("ipv4_multicast_group_address", ui->line_MulticastGroupIPv4->text());

        settings.setValue("ipv6_multicast_group_enabled", ui->check_Channels_IPv6->isChecked());
        settings.setValue("ipv6_multicast_group_address", ui->line_MulticastGroupIPv6->text());

        settings.setValue("channels_autorejoin", ui->check_Channels_AutoRejoin->isChecked());

        settings.setValue("orientation_vertical", ui->radio_Orientation_Vertical->isChecked());
        settings.setValue("direction_downright", ui->radio_Direction_DownRight->isChecked());

        settings.setValue("always_on_top", ui->check_Always_On_Top->isChecked());

        settings.setValue("dash_pos", m_dash_pos);
    settings.endGroup();
}

void Dialog::slot_quit()
{
    // do any cleanup needed...

    m_trayIcon->hide();

    if (m_multicast_receiver)
        m_multicast_receiver.clear();

    save_settings();

    // ...and leave.
    QTimer::singleShot(100, qApp, &QApplication::quit);
}

void Dialog::slot_dash_moved(QPoint pos)
{
    m_dash_pos = pos;
    save_settings();
}

void Dialog::slot_set_control_states()
{
    bool ipv4_enabled{ui->check_Channels_IPv4->isChecked()};
    bool ipv6_enabled{ui->check_Channels_IPv6->isChecked()};

    ui->line_MulticastGroupPort->setEnabled(!m_multicast_group_member);

    ui->check_Channels_IPv4->setEnabled(!m_multicast_group_member);
    ui->line_MulticastGroupIPv4->setEnabled(ipv4_enabled && !m_multicast_group_member);
    ui->button_MulticastGroupIPv4_Randomize->setEnabled(ipv4_enabled && !m_multicast_group_member);

    ui->check_Channels_IPv6->setEnabled(!m_multicast_group_member);
    ui->line_MulticastGroupIPv6->setEnabled(ipv6_enabled && !m_multicast_group_member);
    ui->button_MulticastGroupIPv6_Randomize->setEnabled(ipv6_enabled && !m_multicast_group_member);

    ui->check_Channels_AutoRejoin->setEnabled(!m_multicast_group_member);

    ui->button_Channels_Join->setEnabled(ipv4_enabled || ipv6_enabled);

    ui->radio_Orientation_Vertical->setEnabled(!m_multicast_group_member);
    ui->radio_Orientation_Horizontal->setEnabled(!m_multicast_group_member);

    ui->radio_Direction_DownRight->setEnabled(!m_multicast_group_member);
    ui->radio_Direction_UpLeft->setEnabled(!m_multicast_group_member);

    ui->check_Always_On_Top->setEnabled(!m_multicast_group_member);

    // auto enable_ok = (ui->radio_Orientation_Vertical->isChecked() || ui->radio_Orientation_Horizontal->isChecked()) &&
    //                  (ui->radio_Direction_UpLeft->isChecked() || ui->radio_Direction_DownRight->isChecked());

    // if(supports_push() && ui->check_PUSH->isChecked())
    //     enable_ok = enable_ok && !ui->line_IP->text().isEmpty();

    // QPushButton* ok_button = ui->buttonBox->button(QDialogButtonBox::Ok);
    // ok_button->setEnabled(enable_ok);
}

void Dialog::slot_tray_menu_action(QAction* /*action*/)
{}

void Dialog::slot_tray_message_clicked()
{
    QMessageBox::information(
        nullptr,
        tr("Dash'd"),
        tr("Sorry, I already gave what help I could.\n"
           "Maybe you should try asking a human?"));
}

void Dialog::slot_tray_icon_activated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason)
    {
        case QSystemTrayIcon::Trigger:
            setVisible(!isVisible());
            if (isVisible())
                activateWindow();
            break;

        case QSystemTrayIcon::DoubleClick:
        case QSystemTrayIcon::MiddleClick:
        default:
            break;
    }
}

void Dialog::slot_multicast_group_join()
{
    // save_settings();

    if (m_multicast_group_member)
    {
        ui->button_Channels_Join->setText(tr("Join"));

        m_multicast_receiver.clear();
        m_dashboard.clear();

        // QPushButton* ok_button = ui->buttonBox->button(QDialogButtonBox::Ok);
        // ok_button->setEnabled(true);
    }
    else
    {
        // The dashboard won't be visible unless it has something to display.
        m_dashboard = DashboardPtr(new Dashboard(true, ui->check_Always_On_Top->isChecked(), m_orientation,
            // m_direction will only ever be Down or Up; map that in the Horizontal case
            m_orientation == Dashboard::Orientation::Horizontal ?
                (m_direction == Dashboard::Direction::Down ? Dashboard::Direction::Right: Dashboard::Direction::Left) : m_direction)
        );
        m_dashboard->setGeometry(QRect(m_dash_pos.x(), m_dash_pos.y(), base_symmetry, base_symmetry));
        connect(m_dashboard.data(), &Dashboard::signal_dash_moved, this, &Dialog::slot_dash_moved);

        ui->button_Channels_Join->setText(tr("Leave"));

        auto group_port_str{ui->line_MulticastGroupPort->text()};
        if (group_port_str.isEmpty())
            group_port_str = ui->line_MulticastGroupPort->placeholderText();
        auto group_port{static_cast<uint16_t>(group_port_str.toInt())};

        QString ipv4_multcast_group;
        if (ui->check_Channels_IPv4->isChecked())
        {
            ipv4_multcast_group = ui->line_MulticastGroupIPv4->text();
            if (ipv4_multcast_group.isEmpty())
                ipv4_multcast_group = ui->line_MulticastGroupIPv4->placeholderText();
        }

        QString ipv6_multcast_group;
        if (ui->check_Channels_IPv6->isChecked())
        {
            ipv6_multcast_group = ui->line_MulticastGroupIPv6->text();
            if (ipv6_multcast_group.isEmpty())
                ipv6_multcast_group = ui->line_MulticastGroupIPv6->placeholderText();
        }

        if (m_randomized_addresses && (!ui->line_MulticastGroupIPv4->text().isEmpty() || !ui->line_MulticastGroupIPv6->text().isEmpty()))
        {
            QMessageBox::warning(
                this,
                "Randomized Addresses",
                "You have randomized one or more multicast addresses.\n\n"
                "Please ensure other member machines are using the\n"
                "same new address or your Dash'd may not function.");

            m_randomized_addresses = false;
        }

        m_multicast_receiver.reset(new Receiver(group_port, ipv4_multcast_group, ipv6_multcast_group, this));
        connect(m_multicast_receiver.data(), &Receiver::signal_datagram_available, this, &Dialog::slot_process_peer_event);

        // QPushButton* ok_button = ui->buttonBox->button(QDialogButtonBox::Ok);
        // ok_button->setEnabled(false);
    }

    m_multicast_group_member = !m_multicast_group_member;

    QTimer::singleShot(0, this, &Dialog::slot_set_control_states);
#ifdef TEST
    m_test_count = 0;
    QTimer::singleShot(5000, this, &Dialog::slot_test_insert_sensor);
#endif
}

void Dialog::slot_process_peer_event(const QByteArray& datagram)
{
    auto doc{QJsonDocument::fromJson(datagram)};
    if(!doc.isNull())
    {
        QJsonObject object = doc.object();
        if(object.contains("type"))
        {
            assert(object.contains("domain_id"));
            assert(object.contains("domain_name"));

            auto msg_type = SharedTypes::MsgText2Type[object["type"].toString()];
            auto domain_id = reinterpret_cast<uint64_t>(object["domain_id"].toString().toULong());
            Q_UNUSED(domain_id)
            auto domain_name = QUrl::fromPercentEncoding(object["domain_name"].toString().toUtf8());

            switch(msg_type)
            {
                case SharedTypes::MessageType::Sensor:
                    assert(object.contains("sensor_name"));
                    assert(object.contains("sensor_state"));
                    {
                        auto sensor_name = QUrl::fromPercentEncoding(object["sensor_name"].toString().toUtf8());
                        auto sensor_state = object["sensor_state"].toString().toLower();
                        assert(SharedTypes::MsgText2State.contains(sensor_state));

                        ui->text_Log->appendPlainText(
                            QString("%1::%2::%3")
                                .arg(domain_name, sensor_name, sensor_state)
                        );
                    }
                    break;

                case SharedTypes::MessageType::Offline:
                    assert(object.contains("sensor_name"));
                    // Sensor has gone offline
                    {
                        auto sensor_name = QUrl::fromPercentEncoding(object["sensor_name"].toString().toUtf8());
                        ui->text_Log->appendPlainText(
                            QString("%1::%2::Offline")
                                .arg(domain_name, sensor_name)
                        );
                    }
                    break;

                case SharedTypes::MessageType::Warning:
                    assert(object.contains("sensor_name"));
                    assert(object.contains("domain_warning"));
                    // A warning of some type
                    {
                        auto sensor_name = QUrl::fromPercentEncoding(object["sensor_name"].toString().toUtf8());
                        auto domain_warning = object["domain_warning"].toString();
                        ui->text_Log->appendPlainText(
                            QString("%1::%2::Warning::%3")
                                .arg(domain_name, sensor_name, domain_warning)
                        );
                    }
                case SharedTypes::MessageType::Error:
                    // An error of some type
                    break;
            }
        }
    }
}

void Dialog::slot_randomize_ipv4()
{
    std::random_device rd;
    std::mt19937 rd_mt(rd());
    std::uniform_int_distribution<> byte(0, 255);

    auto b1{byte(rd_mt)};
    auto b2{byte(rd_mt)};
    auto b3{byte(rd_mt)};

    ui->line_MulticastGroupIPv4->setText(QString("239.%1.%2.%3").arg(b1).arg(b2).arg(b3));

    m_randomized_addresses = !m_randomized_addresses && true;
}

void Dialog::slot_randomize_ipv6()
{
    std::random_device rd;
    std::mt19937 rd_mt(rd());
    std::uniform_int_distribution<> byte(1, 65535);

    ui->line_MulticastGroupIPv6->setText(QString("ff12::%1").arg(byte(rd_mt)));

    m_randomized_addresses = !m_randomized_addresses && true;
}

void Dialog::slot_accept_settings()
{
    m_orientation = ui->radio_Orientation_Vertical->isChecked() ?
        Dashboard::Orientation::Vertical :
        Dashboard::Orientation::Horizontal;
    m_direction = ui->radio_Direction_DownRight->isChecked() ?
        Dashboard::Direction::Down :
        Dashboard::Direction::Up;

    save_settings();
    hide();
}

void Dialog::slot_reject_settings()
{
    hide();
}

void Dialog::slot_orientation_vertical()
{
    if(m_orientation != Dashboard::Orientation::Vertical)
    {
        ui->radio_Orientation_Vertical->setChecked(true);
        ui->radio_Orientation_Horizontal->setChecked(false);

        ui->radio_Direction_UpLeft->setText(tr("Up"));
        ui->radio_Direction_DownRight->setText(tr("Down"));
    }

    m_orientation = Dashboard::Orientation::Vertical;

    QTimer::singleShot(0, this, &Dialog::slot_set_control_states);
}

void Dialog::slot_orientation_horizontal()
{
    if(m_orientation != Dashboard::Orientation::Horizontal)
    {
        ui->radio_Orientation_Vertical->setChecked(false);
        ui->radio_Orientation_Horizontal->setChecked(true);

        ui->radio_Direction_UpLeft->setText(tr("Left"));
        ui->radio_Direction_DownRight->setText(tr("Right"));
    }

    m_orientation = Dashboard::Orientation::Horizontal;

    QTimer::singleShot(0, this, &Dialog::slot_set_control_states);
}

void Dialog::slot_direction_downright()
{
    m_direction = Dashboard::Direction::Down;

    ui->radio_Direction_UpLeft->setChecked(false);
    ui->radio_Direction_DownRight->setChecked(true);

    QTimer::singleShot(0, this, &Dialog::slot_set_control_states);
}

void Dialog::slot_direction_upleft()
{
    m_direction = Dashboard::Direction::Up;

    ui->radio_Direction_UpLeft->setChecked(true);
    ui->radio_Direction_DownRight->setChecked(false);

    QTimer::singleShot(0, this, &Dialog::slot_set_control_states);
}
