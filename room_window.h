#ifndef ROOM_WINDOW_H
#define ROOM_WINDOW_H

#include <QWidget>

class QUdpSocket;
class send_window;

namespace Ui {
class room_window;
}

enum message_type {
    messageCome,
    newUser,
    userLeft, 
    fileName,
    refuseName
};

class room_window : public QWidget
{
    Q_OBJECT

public:
    explicit room_window(QWidget *parent = 0);
    ~room_window();

protected:
    void new_user(QString user_name,
                  QString local_hostname,
                  QString ip_address);
    void user_left(QString user_name,
                   QString local_hostname);
    void send_message(message_type type, QString server_address = "");
    
    QString get_ip();
    QString get_username();
    QString get_message();

    void has_pending_file(QString user_name, QString server_addr,
                          QString client_addr, QString file_name);
private:
    Ui::room_window *ui;
    QUdpSocket* udp_socket;
    send_window* server;
    QString file_name;
    qint16 port;
    qint64 last_recv;
    
private slots:
    void process_pending_datagrams();
    void get_file_name(QString);
    void on_send_message_clicked();
    void on_exit_room_clicked();
    void on_send_file_clicked();
};


#endif // ROOM_WINDOW_H
