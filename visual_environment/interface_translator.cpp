#include <QCoreApplication>
#include <QLibraryInfo>

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
        if (QTranslator* currentTranslator = this->applicationTranslator.get()) {
            qApp->removeTranslator(currentTranslator);
        }

        if (QTranslator* currentTranslator = this->qtTranslator.get()) {
            qApp->removeTranslator(currentTranslator);
        }
    }

    void InterfaceTranslator::setLanguage(Language language) {
        int index = static_cast<int>(language);
        if (index == -1) {
            this->loadTranslator(nullptr);
        }
        else if (index >= 0 && index < supportedLocales.size()) {
            this->loadTranslator(&supportedLocales.at(index));
        }
        else {
            qWarning() << "Unsupported language index:" << index;
        }
    }

    void InterfaceTranslator::loadTranslator(const QLocale* locale) {
        constexpr char resourcePath[] = ":/i18n";
        constexpr char resourcePrefix[] = "_";
        constexpr char fileName[] = "translation_visual_environment";
        constexpr char qtTranslationFileName[] = "qt";
        constexpr char resourceSuffix[] = ".qm";

        if (QTranslator* currentTranslator = this->applicationTranslator.get()) {
            qApp->removeTranslator(currentTranslator);
            this->applicationTranslator.reset();
        }

        if (QTranslator* currentTranslator = this->qtTranslator.get()) {
            qApp->removeTranslator(currentTranslator);
            this->qtTranslator.reset();
        }

        if (locale != nullptr) {
            this->applicationTranslator.reset(new QTranslator{});
            if (!this->applicationTranslator->load(
                *locale, 
                fileName, 
                resourcePrefix, 
                resourcePath, 
                resourceSuffix)) {
                qWarning() << "Failed to load own translator for locale:" << locale->system().name();
            }

            this->qtTranslator.reset(new QTranslator{});
            if (!this->qtTranslator->load(
                *locale,
                qtTranslationFileName,
                resourcePrefix,
                QLibraryInfo::path(QLibraryInfo::TranslationsPath),
                resourceSuffix)) {
                qWarning() << "Failed to load Qt's translator for locale:" << locale->system().name();
            }

            qApp->installTranslator(this->qtTranslator.get());
            qApp->installTranslator(this->applicationTranslator.get());
        }

        // It is expected that clients will rely on QTranslator's capabilities,
        // so we don't pass any information about the loaded translator.
        emit retranslateUI();
    }

    void InterfaceTranslator::startupProbeLocales() {
        // If not supported, the application will run in English (default).
        QLocale systemLocale = QLocale::system();
        for (const QLocale& supportedLocale : supportedLocales) {
            if (supportedLocale.language() == systemLocale.language()) {
                this->loadTranslator(&supportedLocale);
                return;
            }
        }

        this->loadTranslator(nullptr);
    }
}
