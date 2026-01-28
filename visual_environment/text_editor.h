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
#include <QPlainTextEdit>
#include <QMenu>
#include <QAction>
#include <QEvent>
#include <QShortcut>
#include <array>

// TODO: Do a further decomposition of this widget into "WorkingTreeView" and "FileTabWidget" classes.
//       Properly handle CTRL+TAB which does not work well with onTabMoved and onCurrentChanged, because
//       these are emitted before the tab is dropped by the user.

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
        ~TextEditor() noexcept override;

        TextEditor(const TextEditor&) = delete;
        TextEditor& operator=(const TextEditor&) = delete;

        TextEditor(TextEditor&&) = delete;
        TextEditor& operator=(TextEditor&&) = delete;

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
        /// <param name="readOnly">If true, opens the file in read-only mode.</param>
        /// <returns>True if the file was opened successfully or is already open; otherwise, false.</returns>
        bool openNewFile(const QString& filePath, bool readOnly = false);

        /// <summary>
        /// Creates a temporary file tab. Temporary files are not associated with any file path on disk.
        /// In addition, they are not monitored for external changes. Trying to save a temporary file
        /// works like "Save As" operation.
        /// </summary>
        void createTemporaryFile();

        /// <summary>
        /// Returns the file path to the currently selected file in the tab widget.
        /// </summary>
        /// <returns>A const reference to a QString containing the path to the current file.</returns>
        const QString& getCurrentFilePath() const noexcept;

        /// <summary>
        /// Saves the file which is currently selected in the tab widget.
        /// </summary>
        bool saveCurrentFile();

        /// <summary>
        /// A convenience function which saves the currently opened file under a new name.
        /// </summary>
        bool saveCurrentFileAs();

        /// <summary>
        /// Closes the file which is currently selected in the tab widget.
        /// </summary>
        bool closeCurrentFile();

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
        /// <returns>True if all files were closed successfully, false if the operation was cancelled.</returns>
        bool closeAllFiles();

        /// <summary>
        /// Checks if any file has been selected.
        /// </summary>
        /// <returns>true if a file has been selected; otherwise, false.</returns>
        bool hasSelectedFile() const noexcept;

    protected:
        virtual void showEvent(QShowEvent* event) override;
        virtual bool event(QEvent* event) override;

    private slots:
        void onWorkingDirectoryContextMenu(const QPoint& position) noexcept;
        void onWorkingDirectoryItemDoubleClicked(const QModelIndex& index) noexcept;

        void onTabSwitchShortcut();
        void onFileChangedOutside(const QString& path) noexcept;

        void onTabCloseRequested(int index) noexcept;
        void onTabMoved(int from, int to) noexcept;
        void onTabChanged(int index) noexcept;

    public slots:
        void onRetranslateUI() noexcept;

    private:
        // So that you can't accidentally change the layout of this widget
        // from outside this class.
        using QWidget::setLayout;

        // TODO: Start using QtConcurrent to offload file writes to background threads.
        //       This will also make autosaving feasible. Right now, I don't have the time to implement it.

        // TODO: Implement messages for different error codes from the QFile operations.
        //       Right now, I just have a generic error message for all file operation errors.

        // TODO: Translate temporary file names. Right now, they are not dynamically translatable.
        //       At the moment, they just retain the name based on the current locale when created.

        struct OpenedFile {
            QString filePath;
            bool isTemporary;

            QString getNormalizedName() const;
        };

        // Allows switching only between current and previous tabs
        std::array<int, 2> tabSelectionHistory{ -1, -1 };
        QShortcut* tabSwitchShortcut{};

        QString workingDirectoryPath;
        QVector<OpenedFile> openFiles;

        QSplitter* splitter{};
        QTabWidget* fileTabs{};
        QTreeView* workingDirectory{};
        QFileSystemWatcher* fileWatcher{};

        // Context menu and actions
        QMenu* workingDirectoryContextMenu{};
        QAction* openAction{};
        QAction* newFileAction{};
        QAction* newDirectoryAction{};
        QAction* removeAction{};

        QPlainTextEdit* getEditorAtIndex(int index);
        bool closeFileAtIndex(int index);
        bool saveFileAtIndex(int index);
        void onFileChangedOutsideRecursive(const QString& path, int depth) noexcept;

        void connectSignalsManually();
        void setupEditorComponents();
        void setupWorkingDirectoryContextMenu();
    };
}

#endif
