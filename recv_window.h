#ifndef RECV_WINDOW_H
#define RECV_WINDOW_H

#include <QDialog>
#include <QHostAddress>
#include <QFile>
#include <QTime>
class QTcpSocket;

namespace Ui {
class recv_window;
}

class recv_window : public QDialog
{
    Q_OBJECT

public:
    explicit recv_window(QWidget *parent = 0);
    ~recv_window();

    void set_host_addr(QHostAddress address);
    void set_file_name(QString file_name);

protected:
    void closeEvent(QCloseEvent*);

private:
    Ui::recv_window *ui;

    QTcpSocket* tcp_connection;
    quint16 block_size;
    QHostAddress host_addr;
    qint16 tcp_port;

    qint64 total_bytes;
    qint64 bytes_recived;
    qint64 bytes_wait;
    qint64 file_name_size;
    QString file_name;
    QFile* file;
    QByteArray in_block;

    QTime time;

private slots:
    void on_cancel_clicked();

    void on_exit_clicked();

    void new_connect();
    void read_message();
    void display_error(QAbstractSocket::SocketError);
};

#endif // RECV_WINDOW_H
