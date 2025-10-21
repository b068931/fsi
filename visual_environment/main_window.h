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
#include "about_application_window.h"
#include "application_styles_manager.h"

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
        // TODO: Add menu action to jump to line/column in the text editor.
        // TODO: Implement a menu which will allow users to change font size, font family, etc. in the text editor.
        // TODO: Implement syntax highlighting for FSI scripts.
        // TODO: Save settings like window size, last opened directory, language, etc.

        Q_OBJECT

    public:
        explicit MainWindow(
            AboutApplicationWindow* aboutWindow,
            Utility::BackgroundService<Components::FSITools::FSIToolsAdapter>* languageService,
            Components::Internationalization::InterfaceTranslator* i18n,
            Components::ApplicationStyle::ApplicationStylesManager* applicationStyle,
            QWidget* parent = nullptr
        );

        ~MainWindow() noexcept override;

        MainWindow(const MainWindow&) = delete;
        MainWindow& operator= (const MainWindow&) = delete;

        MainWindow(MainWindow&&) = delete;
        MainWindow& operator= (MainWindow&&) = delete;

    public slots:
        void onRetranslateUI() noexcept;

    private slots:
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

        void onMenuChangeThemeDark() noexcept;
        void onMenuChangeThemeLight() noexcept;

        void onMenuShortDescription() noexcept;

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
        using LanguageService = Utility::BackgroundService<Components::FSITools::FSIToolsAdapter>;

        // Child windows.
        AboutApplicationWindow* aboutWindow{};

        // Main parts of the main window.
        Ui::MainApplicationWindow ui{};

        LanguageService* languageService{};
        Components::Internationalization::InterfaceTranslator* i18n;
        Components::ApplicationStyle::ApplicationStylesManager* applicationStyle;

        CustomWidgets::TextEditor* editor{};
        CustomWidgets::EnrichedStatusBar* enrichedStatusBar{};

        // State fields.
        bool runAfterTranslation{ false };

        // Configuration methods.
        void setupTextEditor();
        void setupStatusBar();
        void configureDocumentation();
        void connectSignalsManually();
    };
}

#endif
