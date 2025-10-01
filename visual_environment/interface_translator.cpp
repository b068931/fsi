#include <qcoreapplication.h>
#include "interface_translator.h"

namespace Components::Internationalization {
    // English is not listed here, as it is the default language.
    const QList<QLocale> InterfaceTranslator::supportedLocales = {
        QLocale(QLocale::Ukrainian, QLocale::Ukraine)
    };

    InterfaceTranslator::InterfaceTranslator() {
        this->startupProbeLocales();
    }

    InterfaceTranslator::~InterfaceTranslator() noexcept {
        if (QTranslator* currentTranslator = this->activeTranslator.get()) {
            qApp->removeTranslator(currentTranslator);
        }
    }

    void InterfaceTranslator::loadTranslator(const QLocale* locale) {
        constexpr char resourcePath[] = ":/i18n";
        constexpr char resourcePrefix[] = "_";
        constexpr char fileName[] = "translation_visual_environment";
        constexpr char resourceSuffix[] = ".qm";

        if (QTranslator* currentTranslator = this->activeTranslator.get()) {
            qApp->removeTranslator(currentTranslator);
            this->activeTranslator.reset();
        }

        if (locale != nullptr) {
            this->activeTranslator.reset(new QTranslator{});
            if (this->activeTranslator->load(
                *locale, 
                fileName, 
                resourcePrefix, 
                resourcePath, 
                resourceSuffix)) {
                qApp->installTranslator(this->activeTranslator.get());
            }
            else {
                qWarning() << "Failed to load translator for locale:" << *locale;
            }
        }

        // It is expected that clients will rely on QTranslator's capabilities,
        // so we don't pass any information about the loaded translator.
        emit retranslateUI();
    }

    void InterfaceTranslator::startupProbeLocales() {
        // If not supported, the application will run in English (default).
        QLocale systemLocale = QLocale::system();
        for (const QLocale& supportedLocale : InterfaceTranslator::supportedLocales) {
            if (supportedLocale.territory() == systemLocale.territory() && supportedLocale.language() == systemLocale.language()) {
                loadTranslator(&supportedLocale);
                return;
            }
        }
    }
}
