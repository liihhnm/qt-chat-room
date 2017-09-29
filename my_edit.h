#ifndef MY_EDIT_H
#define MY_EDIT_H

#include <QTextEdit>

class my_edit : public QTextEdit
{
    Q_OBJECT
    
public:
    explicit my_edit(QWidget* parent = 0);
    
signals:
    void user_enter_press();
    
protected:
    void keyPressEvent(QKeyEvent* event);
};

#endif // MY_EDIT_H
