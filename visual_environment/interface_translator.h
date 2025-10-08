#ifndef INTERNATIONALIZATION_COMPONENT_H
#define INTERNATIONALIZATION_COMPONENT_H

#include <QLocale>
#include <QTranslator>
#include <QObject>
#include <QScopedPointer>

namespace Components::Internationalization {
    /// <summary>
    /// Component which manages application translation.
    /// Automatically emits the retranslateUI() signal when the language is changed.
    /// </summary>
    class InterfaceTranslator : public QObject {
        Q_OBJECT

    public:
        static const QList<QLocale> supportedLocales;
        enum class Language : int {
            Default = -1,
            English = 0,
            Ukrainian = 1,
        };

        InterfaceTranslator();
        ~InterfaceTranslator() noexcept override;

        InterfaceTranslator(const InterfaceTranslator&) = delete;
        InterfaceTranslator& operator= (const InterfaceTranslator&) = delete;

        InterfaceTranslator(InterfaceTranslator&&) = delete;
        InterfaceTranslator& operator= (InterfaceTranslator&&) = delete;

        /// <summary>
        /// Sets the application's language.
        /// </summary>
        /// <param name="language">The language to set for the application.</param>
        void setLanguage(Language language);

    signals:
        /// <summary>
        /// Signal emitted when the UI needs to be retranslated.
        /// For example, when the user changes the application language.
        /// </summary>
        void retranslateUI();

    private:
        QScopedPointer<QTranslator> applicationTranslator;
        QScopedPointer<QTranslator> qtTranslator;

        /// <summary>
        /// Loads a translator resource by its name.
        /// </summary>
        /// <param name="locale">The name of the translator resource to load. Nullptr for default language (English).</param>
        void loadTranslator(const QLocale* locale);
        void startupProbeLocales();
    };
}

#endif