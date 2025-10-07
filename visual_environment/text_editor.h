#ifndef VISUAL_ENVIRONMENT_TEXT_EDITOR_H
#define VISUAL_ENVIRONMENT_TEXT_EDITOR_H

#include <QTreeView>
#include <QTabWidget>
#include <QString>
#include <QVector>
#include <QWidget>
#include <QSplitter>
#include <QtGlobal>
#include <QFileSystemWatcher>

namespace CustomWidgets {
    /// <summary>
    /// A final QWidget-based class that provides a text editor interface
    /// with basic file browsing capabilities and ability to edit several files.
    /// </summary>
    class TextEditor final : public QWidget
    {
        Q_OBJECT

    public:
        explicit TextEditor(QWidget* parent = nullptr);

        TextEditor(const TextEditor&) = delete;
        TextEditor& operator=(const TextEditor&) = delete;

        TextEditor(TextEditor&&) = delete;
        TextEditor& operator=(TextEditor&&) = delete;

        ~TextEditor() noexcept override;

        /// <summary>
        /// Opens the specified working directory. Closes currently open directory if received an empty string.
        /// </summary>
        /// <param name="directoryPath">The path to the directory to open.</param>
        void openWorkingDirectory(const QString& directoryPath);

        /// <summary>
        /// Retrieves the current working directory path.
        /// </summary>
        /// <returns>A constant reference to a QString containing the path of the current working directory.</returns>
        const QString& getWorkingDirectoryPath() const noexcept;

        /// <summary>
        /// Opens a new file at the specified file path. If the file is already open, it switches to that tab instead.
        /// Otherwise, it opens the file in a new tab.
        /// </summary>
        /// <param name="filePath">The path to the file to be opened.</param>
        /// <returns>A boolean value which indicates whether the operation was successful.</returns>
        void openNewFile(const QString& filePath);

        /// <summary>
        /// Sets the splitter ratio to its default value.
        /// This makes the text editor larger than the working directory view.
        /// The default ratio is approximately 1:6.
        /// </summary>
        void setDefaultSplitterRatio();

        /// <summary>
        /// Closes all open files in the editor.
        /// Prompts the user to save any unsaved changes before closing.
        /// </summary>
        void closeAllFiles();

    protected:
        virtual void showEvent(QShowEvent* event) override;

    private slots:
        void onWorkingDirectoryItemDoubleClicked(const QModelIndex& index);
        void onFileChangedOutside(const QString& path);
        void onTabCloseRequested(int index);
        void onTabMoved(int from, int to);

    public slots:
        void onRetranslateUI();

    private:
        // So that you can't accidentally change the layout of this widget
        // from outside this class.
        using QWidget::setLayout;

        struct OpenedFile {
            QString filePath;
            bool isTemporary;
        };

        QString workingDirectoryPath;
        QVector<OpenedFile> openFiles;

        QSplitter* splitter{};
        QTabWidget* fileTabs{};
        QTreeView* workingDirectory{};
        QFileSystemWatcher* fileWatcher{};

        void closeFileAtIndex(int index);
        void saveFileAtIndex(int index);

        void connectSignalsManually();
        void setupEditorComponents();
    };
}

#endif
