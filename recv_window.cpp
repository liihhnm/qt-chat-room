#include "recv_window.h"
#include "ui_recv_window.h"

#include <QTcpSocket>
#include <QMessageBox>
#include <QDebug>

recv_window::recv_window(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::recv_window)
{
    ui->setupUi(this);

    total_bytes = 0;
    bytes_recived = 0;
    bytes_wait = 0;
    file_name_size = 0;

    tcp_connection = new QTcpSocket(this);
    tcp_port = 7777;
    connect(tcp_connection, &QTcpSocket::readyRead,
            this, &recv_window::read_message);
    void (QAbstractSocket::*func_ptr)(QAbstractSocket::SocketError) = &QTcpSocket::error;
    connect(tcp_connection, func_ptr,
            this, &recv_window::display_error);
}

recv_window::~recv_window()
{
    delete ui;
}

void recv_window::set_host_addr(QHostAddress address)
{
    host_addr = address;
    new_connect();
}

void recv_window::set_file_name(QString file_name)
{
    file = new QFile(file_name);
}

void recv_window::closeEvent(QCloseEvent *)
{
    on_exit_clicked();
}

//slots
void recv_window::on_cancel_clicked()
{
    tcp_connection->abort();
    if (file->isOpen())
        file->close();
}

void recv_window::on_exit_clicked()
{
    tcp_connection->close();
    if (file->isOpen())
        file->close();
}

void recv_window::new_connect()
{
    block_size = 0;
    tcp_connection->abort();
    tcp_connection->connectToHost(host_addr, tcp_port);
    time.start();
}

void recv_window::read_message()
{
    QDataStream in(tcp_connection);
    in.setVersion(QDataStream::Qt_5_0);

    float use_time = time.elapsed();

    if (bytes_recived <= sizeof(qint64) * 2)
    {
        if (tcp_connection->bytesAvailable() >= sizeof(qint64) * 2
                && file_name_size == 0)
        {
            in >> total_bytes >> file_name_size;
            bytes_recived += sizeof(qint64) * 2;
        }
        if ((tcp_connection->bytesAvailable() >= file_name_size)
                && file_name_size != 0)
        {
            in >> file_name;
            bytes_recived += file_name_size;

            if (!file->open(QFile::WriteOnly))
            {
                QMessageBox::warning(this, tr("Recive File"),
                                     tr("Can't load file %1\n%2")
                                     .arg(file_name)
                                     .arg(file->errorString()));
                return;
            }
            else
            {
                return;
            }
        }
    }
    if (bytes_recived < total_bytes)
    {
        bytes_recived += tcp_connection->bytesAvailable();
        in_block = tcp_connection->readAll();
        file->write(in_block);
        in_block.resize(0);
    }
    ui->progress_bar->setMaximum(total_bytes);
    ui->progress_bar->setValue(bytes_recived);

    double speed = bytes_recived / use_time;
    ui->tip_label->setText(tr("%1Mb / %2 Mb\t(%3Mb/s)"
                              "\ntime used: %4s")
                           .arg(bytes_recived / (1024 * 1024))
                           .arg(total_bytes / (1024 * 1024))
                           .arg(speed * 1000 / (1024 * 1024), 0, 'f', 2)
                           .arg(use_time / 1000, 0, 'f', 0));

    if (bytes_recived == total_bytes)
    {
        file->close();
        tcp_connection->close();
        ui->tip_label->setText(tr("recive %1 complete")
                               .arg(file_name));
    }
}

void recv_window::display_error(QAbstractSocket::SocketError socket_error)
{
    switch(socket_error)
    {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    default:
        qDebug() << tcp_connection->errorString();
        break;
    }
}
