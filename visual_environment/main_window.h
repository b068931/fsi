#ifndef QT_VISUAL_ENVIRONMENT_MAIN_WINDOW_H
#define QT_VISUAL_ENVIRONMENT_MAIN_WINDOW_H

#include <QtWidgets/QMainWindow>
#include <QEvent>
#include <QCloseEvent>
#include <QShowEvent>
#include <QtGlobal>
#include <QString>
#include <QScopedPointerDeleteLater>
#include <QScopedPointer>

#include "ui_main_window.h"
#include "text_editor.h"
#include "enriched_status_bar.h"
#include "interface_translator.h"
#include "fsi_tools_adapter.h"
#include "background_service.h"

namespace Windows {
    /// <summary>
    /// MainWindow provides the main access point
    /// to the capabilities of the application. It manages
    /// a simple text editor widget (for more information see text_editor.h)
    /// and several child processes started on demand.
    /// Essentially, this class is a mediator between all others.
    /// </summary>
    class MainWindow final : public QMainWindow
    {
        // TODO: Implement a menu which will allow users to switch themes (light/dark).
        // TODO: Implement a menu which will allow users to change font size, font family, etc. in the text editor.
        // TODO: Implement syntax highlighting for FSI scripts.
        // TODO: Save settings like window size, last opened directory, language, etc.

        Q_OBJECT

    public:
        explicit MainWindow(QWidget* parent = nullptr);
        ~MainWindow() noexcept override;

        MainWindow(const MainWindow&) = delete;
        MainWindow& operator= (const MainWindow&) = delete;

        MainWindow(MainWindow&&) = delete;
        MainWindow& operator= (MainWindow&&) = delete;

    private slots:
        void onRetranslateUI() noexcept;

        void onMenuFileNew() noexcept;
        void onMenuFileOpen() noexcept;
        void onMenuFileSave() noexcept;
        void onMenuFileSaveAs() noexcept;
        void onMenuFileClose() noexcept;

        void onMenuWorkingDirectoryOpen() noexcept;
        void onMenuWorkingDirectoryClose() noexcept;

        void onProgramTranslate() noexcept;
        void onRunLastTranslatedProgram() noexcept;
        void onTranslateAndRun() noexcept;
        void onRuntimeEnvironmentLogs() noexcept;

        void onMenuLanguageUkrainian() noexcept;
        void onMenuLanguageEnglish() noexcept;

        void onProgramTranslatorStarted() noexcept;
        void onProgramTranslationResult(
            int exitCode, 
            Components::FSITools::FSIToolsAdapter::ChildResult result
        ) noexcept;

        void onExecutionEnvironmentStarted() noexcept;
        void onExecutionEnvironmentResult(
            int exitCode, 
            Components::FSITools::FSIToolsAdapter::ChildResult result
        ) noexcept;

    protected:
        virtual bool event(QEvent* event) override;
        virtual void closeEvent(QCloseEvent* event) override;
        virtual void showEvent(QShowEvent* event) override;

    private:
        // Main parts of the main window.
        Ui::MainWindowClass ui{};
        Utility::BackgroundService<
            Components::FSITools::FSIToolsAdapter
        > languageService{};

        QScopedPointer<
            Components::Internationalization::InterfaceTranslator,
            QScopedPointerDeleteLater
        > i18n{};

        CustomWidgets::TextEditor* editor{};
        CustomWidgets::EnrichedStatusBar* enrichedStatusBar{};

        // State fields.
        bool runAfterTranslation{ false };

        // Configuration methods.
        void setupTextEditor();
        void setupStatusBar();
        void connectSignalsManually();
    };
}

#endif
