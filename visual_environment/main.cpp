#ifndef NDEBUG
//#include <vld.h>
#endif

#include <QDir>
#include <QFile>
#include <QObject>
#include <QTextStream>
#include <QStyleFactory>
#include <QtWidgets/QApplication>

#include "main_window.h"
#include "about_application_window.h"
#include "interface_translator.h"
#include "fsi_tools_adapter.h"
#include "application_styles_manager.h"
#include "background_service.h"

// TODO: Consider adding command line arguments for opening specific files at startup.
// TODO: Consider implementing some mechanism to use exceptions for error handling.
//       Qt has very poor support for exceptions, but it might be possible to implement something.
//       For example, you are not allowed to throw exceptions from slots, but you can catch them before they leave the slot.
//       It might be possible to implement some mechanism which will automatically catch exceptions and push them to event queue, and then process them (show message box, etc.) in the main event loop.

int main(int argc, char *argv[]) {
    constexpr const char applicationBaseStyle[]{ "Fusion" };

    // Setup application's main scope: QApplication, windows, services, main components, etc.
    QApplication application(argc, argv);
    if (QStyleFactory::keys().contains(applicationBaseStyle)) {
        QApplication::setStyle(QStyleFactory::create(applicationBaseStyle));
    }

    Components::Internationalization::InterfaceTranslator i18n;
    Utility::BackgroundService<Components::FSITools::FSIToolsAdapter> languageService;
    Components::ApplicationStyle::ApplicationStylesManager applicationStyle(
        Components::ApplicationStyle::ApplicationStylesManager::Style::Dark
    );

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
