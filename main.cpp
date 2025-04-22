#include <QApplication>
#include <QDebug>
#include <QDialog>
#include <QLayout>
#include <QLineEdit>
#include <QPointer>
#include <QVBoxLayout>

class TestDialog : public QDialog
{
    Q_OBJECT
public:
    TestDialog(QWidget* parent) :  QDialog(parent)
    {
        m_line_edit = new QLineEdit();
        m_line_edit->setInputMethodHints(Qt::ImhSensitiveData |
                                         Qt::ImhNoAutoUppercase |
                                         Qt::ImhNoPredictiveText);

#ifdef Q_OS_ANDROID
        m_line_edit->installEventFilter(this);
#endif
        connect(m_line_edit, &QLineEdit::textEdited, this,
                &TestDialog::processChangedText);

        auto mainlayout = new QVBoxLayout();
        mainlayout->addWidget(m_line_edit);
        mainlayout->addStretch(1);

        setLayout(mainlayout);
        m_old_text = "";
    }

#ifdef Q_OS_ANDROID
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (obj != m_line_edit || event->type() != QEvent::InputMethod)
            return false;

        if (m_ignoreNextInputEvent) {
            m_ignoreNextInputEvent = false;
            event->ignore();
            return true;
        }

        return false;
    }
#endif

protected:
    QPointer<QLineEdit> m_line_edit;

    void processChangedText()
    {
        // Automatically strip white space and insert the dashes, because it's a
        // pain having to do that on a mobile on-screen keyboard
        QString initial_text = m_line_edit->text();

        const bool cursor_at_end = (m_line_edit->cursorPosition() == initial_text.length());

        QString new_text = initial_text.trimmed();

        // Only add a dash when cursor is at the end and we're not deleting...
        if (cursor_at_end && new_text.startsWith(m_old_text)) {
            //                  1111111
            //        01234567890123456
            //        kidil-sovib-dufob-hivol-nutab-linuj-kivad-nozov-t
            //            ^     ^     ^                               ^
            // Len        5    11    17 ...                          49
            // Prev dash -1     5    11

            // ...or beyond the maximum length
            const int max_len = 8 * 6 + 1;  // 8 groups of 5-and-dash, then check
            if (new_text.length() < max_len) {
                int prev_dash_pos = int(new_text.lastIndexOf('-'));
                if ((new_text.length() - prev_dash_pos) == 6) {
                    new_text += '-';
                }
            }
        }

        // Set text will put the cursor to the end so only set it if it has changed
        if (new_text != initial_text) {
#ifdef Q_OS_ANDROID
            maybeIgnoreNextInputEvent();
#endif
            m_line_edit->setText(new_text);
        }

        m_old_text = new_text;
    }
private:
    QString m_old_text;

#ifdef Q_OS_ANDROID
    bool m_ignoreNextInputEvent = false;

    void maybeIgnoreNextInputEvent()
    {
        if (QGuiApplication::inputMethod()->isVisible())
            m_ignoreNextInputEvent = true;
    }
#endif

};

#if defined __clang__  // NB defined in Qt Creator; put this first for that reason
    #define COMPILER_IS_CLANG
    #if __clang_major__ >= 10
        #define CLANG_AT_LEAST_10
    #endif
#elif defined __GNUC__  // __GNUC__ is defined for GCC and clang
    #define COMPILER_IS_GCC
    #if __GNUC__ >= 7  // gcc >= 7.0
        #define GCC_AT_LEAST_7
    #endif
#elif defined _MSC_VER
    #define COMPILER_IS_VISUAL_CPP
#endif


#if defined COMPILER_IS_CLANG || defined COMPILER_IS_GCC
    #define VISIBLE_SYMBOL __attribute__ ((visibility ("default")))
#elif defined COMPILER_IS_VISUAL_CPP
    #define VISIBLE_SYMBOL __declspec(dllexport)
#else
    #error "Don't know how to enforce symbol visibility for this compiler!"
#endif


VISIBLE_SYMBOL int main(int argc, char* argv[]){
    QApplication app(argc, argv);
    TestDialog dialog(nullptr);
    dialog.exec();

    return app.exec();
}

#include "main.moc"
