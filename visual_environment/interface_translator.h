#ifndef INTERNATIONALIZATION_COMPONENT_H
#define INTERNATIONALIZATION_COMPONENT_H

#include <QLocale>
#include <QTranslator>
#include <qobject.h>
#include <memory>

namespace Components::Internationalization {
    class InterfaceTranslator : public QObject {
        Q_OBJECT

    public:
        InterfaceTranslator();
        ~InterfaceTranslator() noexcept;

        InterfaceTranslator(const InterfaceTranslator&) = delete;
        InterfaceTranslator& operator= (const InterfaceTranslator&) = delete;

        InterfaceTranslator(InterfaceTranslator&&) = delete;
        InterfaceTranslator& operator= (InterfaceTranslator&&) = delete;

    public:
        /// <summary>
        /// Loads a translator resource by its name.
        /// Note that you must manually emit the retranslateUI() signal.
        /// </summary>
        /// <param name="locale">The name of the translator resource to load. Nullptr for default language (English).</param>
        void loadTranslator(const QLocale* locale);

    signals:
        /// <summary>
        /// Signal emitted when the UI needs to be retranslated.
        /// For example, when the user changes the application language.
        /// </summary>
        void retranslateUI();

    private:
        static const QList<QLocale> supportedLocales;

        std::unique_ptr<QTranslator> activeTranslator;
        void startupProbeLocales();
    };
}

#endif