#include "my_edit.h"
#include <QKeyEvent>

my_edit::my_edit(QWidget* parent) :
    QTextEdit(parent)
{
}

void my_edit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return)
    {
        if (event->modifiers() == Qt::ControlModifier)
            textCursor().insertText("\n");
        else if (event->modifiers() == Qt::NoModifier)
            emit user_enter_press();
    }
    else
        QTextEdit::keyPressEvent(event);
}
