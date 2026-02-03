#ifdef APPLICATION_RELEASE
#define STARTUP_COMPONENTS_HIDE_APPLICATION_CONSOLE
#endif

#ifndef NDEBUG
//#include <vld.h>
#endif

#include <QDir>
#include <QFile>
#include <QObject>
#include <QTextStream>
#include <QStyleFactory>
#include <QtLogging>
#include <QtWidgets/QApplication>

#include "main_window.h"
#include "about_application_window.h"
#include "interface_translator.h"
#include "fsi_tools_adapter.h"
#include "application_styles_manager.h"
#include "background_service.h"

#include "../startup_components/startup_definitions.h"
#include "../startup_components/local_crash_handlers.h"

// TODO: Consider adding command line arguments for opening specific files at startup.

APPLICATION_ENTRYPOINT("VISUAL ENVIRONMENT", FSI_PROJECT_VERSION, argc, argv) {
    assert(argc > 0 && "Must contain at least one argument.");
    crash_handling::install_local_crash_handlers();

    constexpr char applicationBaseStyle[]{ "Fusion" };
    constexpr char fontsRootDirectory[]{ ":/fonts" };

    // Similar message pattern to "module_mediator".
    qSetMessagePattern("[%{type}] [TIME: %{time process}] " \
        "[FILE: %{file}, FILE LINE: %{line}, FUNCTION NAME: %{function}] %{message}");

    // Setup application's main scope: QApplication, windows, services, main components, etc.
    QApplication application(argc, argv);
    if (QStyleFactory::keys().contains(applicationBaseStyle)) {
        QApplication::setStyle(QStyleFactory::create(applicationBaseStyle));
    }

    // Set up style component first, so that if other components want to create widgets 
    // during their initialization, they will have the correct style applied.
    Components::ApplicationStyle::ApplicationStylesManager applicationStyle(
        Components::ApplicationStyle::ApplicationStylesManager::Style::Dark,
        fontsRootDirectory
    );

    Components::Internationalization::InterfaceTranslator i18n;
    Utility::BackgroundService<Components::FSITools::FSIToolsAdapter> languageService;

    Windows::AboutApplicationWindow aboutWindow;
    Windows::MainWindow mainWindow(&aboutWindow, &languageService, &i18n, &applicationStyle);

    // Create global connections.
    QObject::connect(&i18n, &Components::Internationalization::InterfaceTranslator::retranslateUI,
        [&languageService] {
            // For the language service we'll have to use this questionable approach.
            languageService.send([] (Components::FSITools::FSIToolsAdapter* adapter) {
                adapter->onRetranslateUI();
            });
        });

    languageService.start();
    mainWindow.showMaximized();

    return QApplication::exec();
}
