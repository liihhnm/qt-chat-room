#ifndef SEND_WINDOW_H
#define SEND_WINDOW_H

#include <QDialog>
#include <QTime>

class QFile;
class QTcpServer;
class QTcpSocket;

namespace Ui {
class send_window;
}

class send_window : public QDialog
{
    Q_OBJECT

public:
    explicit send_window(QWidget *parent = 0);
    ~send_window();

    void init_send();
    void refuse();

protected:
    void closeEvent(QCloseEvent*);

private:
    Ui::send_window *ui;

    qint16 tcp_port;
    QTcpServer* tcp_server;
    QString  file_name;
    QString the_file;

    QFile* file;

    qint64 total_bytes;
    qint64 bytes_written;
    qint64 bytes_left;
    qint64 payload_size;
    QByteArray out_block;

    QTcpSocket* tcp_connection;
    QTime time;

signals:
    void send_filename(QString file_name);

private slots:
    void send_message();
    void update_progress(qint64 bytes);
    void on_open_file_clicked();

    void on_send_file_clicked();

    void on_cancel_send_clicked();

};

#endif // SEND_WINDOW_H
