#include <qcoreapplication.h>
#include "interface_translator.h"

namespace Components::Internationalization {
    const QList<QLocale> InterfaceTranslator::supportedLocales = {
        // Default locale (English). Does not differ from the strings in the source code.
        QLocale(QLocale::English, QLocale::UnitedKingdom),
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

    void InterfaceTranslator::setLanguage(Language language) {
        int index = static_cast<int>(language);
        if (index == -1) {
            this->loadTranslator(nullptr);
        }
        else if (index >= 0 && index < InterfaceTranslator::supportedLocales.size()) {
            this->loadTranslator(&InterfaceTranslator::supportedLocales.at(index));
        }
        else {
            qWarning() << "Unsupported language index:" << index;
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
            if (supportedLocale.language() == systemLocale.language()) {
                this->loadTranslator(&supportedLocale);
                return;
            }
        }

        this->loadTranslator(nullptr);
    }
}
