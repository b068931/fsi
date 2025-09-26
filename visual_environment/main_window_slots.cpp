#include <QStatusTipEvent>
#include "main_window.h"

bool MainWindow::event(QEvent* event) {
    if (event->type() == QEvent::StatusTip) {
        QStatusTipEvent* statusTipEvent = dynamic_cast<QStatusTipEvent*>(event);

        Q_ASSERT(statusTipEvent != nullptr && "Unexpected: the event is not of type QStatusTipEvent.");
        this->enrichedStatusBar->toolTip(
            statusTipEvent->tip()
        );

        return true;
    }

    return QMainWindow::event(event);
}

