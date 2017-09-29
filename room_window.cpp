/*
*   udp broadcast datagram format:
*   type info --> user name --> host name --> [ip address] --> [message]
*/

#include "room_window.h"
#include "ui_room_window.h"
#include "my_edit.h"
#include "send_window.h"
#include "recv_window.h"

#include <QUdpSocket>
#include <QHostInfo>
#include <QKeyEvent>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QDateTime>
#include <QProcessEnvironment>
#include <QDateTime>
#include <QFileDialog>

#include <vector>
using std::vector;



room_window::room_window(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::room_window)
{
    ui->setupUi(this);
    this->setWindowTitle("LAN chat room");
    ui->message_editor->setFocus();
    connect(ui->message_editor, &my_edit::user_enter_press,
            this, &room_window::on_send_message_clicked);

    udp_socket = new QUdpSocket(this);
    port = 46666;
    last_recv = 0;
    udp_socket->bind(port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    connect(udp_socket, &QUdpSocket::readyRead,
            this, &room_window::process_pending_datagrams);
    send_message(newUser);

    server = new send_window(this);
    connect(server, &send_window::send_filename,
            this, &room_window::get_file_name);
}

room_window::~room_window()
{
    delete ui;
}

void room_window::new_user(QString user_name, QString local_hostname, QString ip_address)
{
    bool user_not_exist = ui->user_list->findItems(local_hostname, Qt::MatchExactly).empty();
    if (user_not_exist)
    {
        typedef QTableWidgetItem Item;
        Item *user = new Item(user_name);
        Item *host = new Item(local_hostname);
        Item *ip = new Item(ip_address);

        ui->user_list->insertRow(0);
        ui->user_list->setItem(0, 0, user);
        ui->user_list->setItem(0, 1, host);
        ui->user_list->setItem(0, 2, ip);

        ui->message_browser->setAlignment(Qt::AlignCenter);
        ui->message_browser->setTextColor(Qt::gray);
        ui->message_browser->setCurrentFont(QFont("Times New Roman", 9));
        QString current_time = QDateTime::currentDateTime().toString("hh:mm:ss\t");
        QString notifacation =  current_time + user_name + " online";
        ui->message_browser->append(notifacation);

        ui->online_number->setText(tr("online user(s): %1").arg(ui->user_list->rowCount()));

        send_message(newUser);
    }
}

void room_window::user_left(QString user_name, QString local_hostname)
{
    int index = ui->user_list->findItems(local_hostname, Qt::MatchExactly).first()->row();
    ui->user_list->removeRow(index);

    ui->message_browser->setAlignment(Qt::AlignCenter);
    ui->message_browser->setTextColor(Qt::gray);
    ui->message_browser->setCurrentFont(QFont("Times New Roman", 8));
    QString current_time = QDateTime::currentDateTime().toString("hh:mm:ss\t");
    QString notifacation =  current_time + user_name + " leave";
    ui->message_browser->append(notifacation);

    ui->online_number->setText(tr("online user(s): %1").arg(ui->user_list->rowCount()));
}


void room_window::send_message(message_type type, QString server_address)
{
    QByteArray data;
    QDataStream output(&data, QIODevice::WriteOnly);

    output << type << get_username() << QHostInfo::localHostName();

    switch (type)
    {
    case messageCome:
        if (ui->message_editor->toPlainText().size() != 0)
        {
           output << get_ip() << get_message();
           //ui->message_browser->
        }
        break;

    case newUser:
        output << get_ip();
        break;

    case userLeft:
        break;

    case refuseName:
        output << server_address;
        break;

    case fileName:
        int row = ui->user_list->currentRow();
        QString client_addr = ui->user_list->item(row, 2)->text();
        output << get_ip() << client_addr << file_name;
        break;
    }
    udp_socket->writeDatagram(data, data.length(),
                              QHostAddress::Broadcast, port);
}


//some facility functions
QString room_window::get_ip()
{
    QList<QHostAddress> list = QNetworkInterface::allAddresses();
    foreach (QHostAddress address, list) {
        if(address.protocol() == QAbstractSocket::IPv4Protocol)
            return address.toString();
    }
    return 0;
}



QString room_window::get_username()
{
    vector<QString> entries{"USERNAME", "USER", "HOSTNAME",
                            "USERDOMAIN", "DOMAINNAME"};
    auto env = QProcessEnvironment::systemEnvironment();
    for (auto entry : entries)
    {
        if (env.value(entry) != "")
            return env.value(entry);
    }
    return "unkown";
}

QString room_window::get_message()
{
    QString message = ui->message_editor->toHtml();
    ui->message_editor->clear();
    //ui->message_editor->setFocus();
    return message;
}

void room_window::has_pending_file(QString user_name, QString server_addr,
                                   QString client_addr, QString file_name)
{
    QString ip = get_ip();
    if (ip == client_addr)
    {
        int button = QMessageBox::information(this, tr("recive file"),
                                              tr("from %1:\t%2, recive?")
                                              .arg(user_name)
                                              .arg(file_name),
                                              QMessageBox::Yes, QMessageBox::No);
        if (button == QMessageBox::Yes)
        {
            QString name = QFileDialog::getSaveFileName(0, tr("Save file"), file_name);
            if (!name.isEmpty())
            {
                recv_window* client = new recv_window(this);
                client->set_file_name(name);
                client->set_host_addr(QHostAddress(server_addr));
                client->show();
            }
        }
        else
        {
            send_message(refuseName, server_addr);
        }
    }
}


//private slots
void room_window::process_pending_datagrams()
{
    while (udp_socket->hasPendingDatagrams())
    {
        QByteArray data;
        data.resize(udp_socket->pendingDatagramSize());
        udp_socket->readDatagram(data.data(), data.size());
        QDataStream in(&data, QIODevice::ReadOnly);
        int type;
        in >> type;
        QString user_name, host_name, ip_addr, message;
        QDateTime current_time = QDateTime::currentDateTime();

        switch (type)
        {
        case messageCome:
            in >> user_name >> host_name >> ip_addr >> message;
            if (current_time.toSecsSinceEpoch() - last_recv > 180)
            {
                ui->message_browser->setTextColor(QColor(0, 66, 98));
                ui->message_browser->setCurrentFont(QFont("Times New Roman", 9));
                ui->message_browser->append(current_time.toString("MM-dd hh:mm:ss"));
            }
            last_recv = current_time.toSecsSinceEpoch();
            if (ip_addr == get_ip())
                ui->message_browser->setAlignment(Qt::AlignRight);
            else
                ui->message_browser->setAlignment(Qt::AlignLeft);

            ui->message_browser->setTextColor(Qt::blue);
            ui->message_browser->setCurrentFont(QFont("Times New Roman",10));
            ui->message_browser->append("[ " +user_name+" ] ");
            ui->message_browser->append(message);
            break;

        case newUser:
            in >> user_name >> host_name >> ip_addr;
            new_user(user_name, host_name, ip_addr);
            break;

        case userLeft:
            in >> user_name >> host_name;
            user_left(user_name, host_name);
            break;

        case fileName: {
            in >> user_name >> host_name >> ip_addr;
            QString client_addr, cur_file_name;
            in >> client_addr >> cur_file_name;
            has_pending_file(user_name, ip_addr, 
                             client_addr, cur_file_name);
            break;
        }

        case refuseName:
            in >> user_name >> host_name;
            QString server_addr;
            in >> server_addr;
            if (server_addr == get_ip())
                server->refuse();
            break;
        }
    }
}

void room_window::get_file_name(QString name)
{
    file_name = name;
    send_message(fileName);
}


void room_window::on_send_message_clicked()
{
    send_message(messageCome);
}

void room_window::on_exit_room_clicked()
{
    close();
}

void room_window::on_send_file_clicked()
{
    if (ui->user_list->selectedItems().isEmpty())
    {
        QMessageBox::warning(0, tr("Select User"),
                             tr("Please choose one user in the list first!"),
                             QMessageBox::Ok);
        return;
    }

    server->show();
    server->init_send();
}
