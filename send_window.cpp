#include "send_window.h"
#include "ui_send_window.h"

#include <QFile>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFileDialog>
#include <QMessageBox>

send_window::send_window(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::send_window)
{
    ui->setupUi(this);

    tcp_port = 7777;
    tcp_server = new QTcpServer(this);
    connect(tcp_server, &QTcpServer::newConnection,
            this, &send_window::send_message);

    init_send();
}

send_window::~send_window()
{
    delete ui;
}

void send_window::init_send()
{
    payload_size = 32 * 1024;
    total_bytes = 0;
    bytes_left = 0;
    bytes_written = 0;

    ui->tip_label->setText(tr("choose the file to be send"));
    ui->progress_bar->reset();
    ui->open_file->setEnabled(true);
    ui->send_file->setEnabled(false);

    tcp_server->close();
}

void send_window::refuse()
{
    tcp_server->close();
    ui->tip_label->setText(tr("refused to accept!"));
}

void send_window::closeEvent(QCloseEvent *)
{
    on_cancel_send_clicked();
}

void send_window::send_message()
{
    ui->send_file->setEnabled(false);
    tcp_connection = tcp_server->nextPendingConnection();
    connect(tcp_connection, &QTcpSocket::bytesWritten,
            this, &send_window::update_progress);

    ui->tip_label->setText(tr("start sending %1").arg(the_file));
    file = new QFile(file_name);
    if (!file->open(QFile::ReadOnly))
    {
        QMessageBox::warning(this, tr("Open file"),
                             tr("Can't read file %1\n%2")
                             .arg(file_name)
                             .arg(file->errorString()));
        return;
    }

    total_bytes = file->size();
    QDataStream send_out(&out_block, QIODevice::WriteOnly);
    //send_out.setVersion(QDataStream::Qt_5_0);

    time.start();
    QString current_file = file_name.right(file_name.size() - file_name.lastIndexOf('/') - 1);
    send_out << qint64(0) << qint64(0) << current_file;
    total_bytes += out_block.size();

    send_out.device()->seek(0);
    send_out << total_bytes << qint64(out_block.size() - sizeof(qint64) * 2);

    bytes_left = total_bytes - tcp_connection->write(out_block);
    out_block.resize(0);
}

void send_window::update_progress(qint64 bytes)
{
    qApp->processEvents();
    bytes_written += (int)bytes;
    if (bytes_left > 0)
    {
        out_block = file->read(qMin(bytes_left, payload_size));
        bytes_left -= (int)tcp_connection->write(out_block);
        out_block.resize(0);
    }
    else
        file->close();

    ui->progress_bar->setMinimum(total_bytes);
    ui->progress_bar->setValue(bytes_written);

    float use_time = time.elapsed();
    double speed = bytes_written / use_time;
    ui->tip_label->setText(tr("%1Mb / %2 Mb\t(%3Mb/s)"
                              "\ntime used: %4s")
                           .arg(bytes_written / (1024 * 1024))
                           .arg(total_bytes / (1024 * 1024))
                           .arg(speed * 1000 / (1024 * 1024), 0, 'f', 2)
                           .arg(use_time / 1000, 0, 'f', 0));
    if (bytes_written == total_bytes)
    {
        file->close();
        tcp_server->close();
        ui->tip_label->setText(tr("transport file %1 success").arg(the_file));

    }
}

void send_window::on_open_file_clicked()
{
    file_name = QFileDialog::getOpenFileName(this);
    if (!file_name.isEmpty())
    {
        the_file = file_name.right(file_name.size() - file_name.lastIndexOf('/') - 1);
        ui->tip_label->setText(tr("file to be send: %1").arg(the_file));
        ui->send_file->setEnabled(true);
        ui->open_file->setEnabled(false);
    }
}

void send_window::on_send_file_clicked()
{
    if (!tcp_server->listen(QHostAddress::Any, tcp_port))
    {
        close();
        return;
    }

    ui->tip_label->setText(tr("wait for reciving..."));
    emit send_filename(the_file);
}

void send_window::on_cancel_send_clicked()
{
    if (tcp_server->isListening())
    {
        tcp_server->close();
        if (file->isOpen())
            file->close();
        tcp_connection->abort();

    }
    close();
}
