#ifndef VISUAL_ENVIRONMENT_TEXT_EDITOR_H
#define VISUAL_ENVIRONMENT_TEXT_EDITOR_H

#include <QTreeView>
#include <QPair>
#include <QTabWidget>
#include <QString>
#include <QVector>
#include <QWidget>
#include <QSplitter>
#include <QtGlobal>

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

    virtual void showEvent(QShowEvent* event) override;

public:
    /// <summary>
    /// Opens the specified working directory.
    /// </summary>
    /// <param name="directoryPath">The path to the directory to open.</param>
    void openWorkingDirectory(const QString& directoryPath);

    /// <summary>
    /// Retrieves the currently selected file and its selection status.
    /// </summary>
    /// <returns>A QPair containing the name of the currently selected file (QString) 
    /// and a boolean indicating whether the file has been saved on disk.</returns>
    QPair<QString, bool> getCurrentlySelectedFile() const;

    /// <summary>
    /// Sets the splitter ratio to its default value.
    /// This makes the text editor larger than the working directory view.
    /// The default ratio is approximately 1:6.
    /// </summary>
    void setDefaultSplitterRatio();

private slots:
    void onWorkingDirectoryItemDoubleClicked(const QModelIndex& index);
    void onTabCloseRequested(int index);
    void onTabMoved(int from, int to);

private:
    void openNewFile(const QString& filePath);
    void closeFileAtIndex(int index);

private:
    // So that you can't accidentally change the layout of this widget
    // from outside this class.
    using QWidget::setLayout;

    struct OpenedFile {
        QString filePath;
        bool isTemporary;
    };

    QVector<OpenedFile> openFiles;

    QSplitter* splitter{};
    QTabWidget* fileTabs{};
    QTreeView* workingDirectory{};

    void connectSignalsManually();
    void setupEditorComponents();
};

#endif
