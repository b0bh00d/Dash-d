#pragma once

#include <QUdpSocket>
#include <QHostAddress>
#include <QSharedPointer>

class Sender : public QObject
{
    Q_OBJECT

public:
    explicit Sender(uint16_t group_port, const QString& ipv4_group, const QString& ipv6_group, QObject* parent = nullptr);

    void send_datagram(const QByteArray& datagram);

private:
    QUdpSocket m_udp_socket_ipv4;
    QUdpSocket m_udp_socket_ipv6;

    QHostAddress m_group_address_ipv4;
    QHostAddress m_group_address_ipv6;

    uint16_t m_group_port{0};
};

using SenderPtr = QSharedPointer<Sender>;
