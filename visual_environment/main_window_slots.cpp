#include <QFileSystemModel>
#include <QFileInfo>

#include "main_window.h"

void MainWindow::onWorkingDirectoryItemDoubleClicked(const QModelIndex &index) {
    QFileSystemModel* fileModel = qobject_cast<QFileSystemModel*>(workingDirectory->model());
    if (!fileModel)
        return;
        
    QString filePath = fileModel->filePath(index);
    QFileInfo fileInfo(filePath);
    
    if (fileInfo.isDir()) {
        if (workingDirectory->isExpanded(index))
            workingDirectory->collapse(index);
        else
            workingDirectory->expand(index);
    }
    else if (fileInfo.isFile()) {
        // If it's a file, open it in the editor
        // You'll need to implement a method to open files in your text editor
        (void)0;
    }
}
