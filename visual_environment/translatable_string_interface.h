#ifndef ITRANSLATABLE_STRING_INTERNATIONALIZATION_COMPONENT_H
#define ITRANSLATABLE_STRING_INTERNATIONALIZATION_COMPONENT_H

#include <QString>

namespace Components::Internationalization {
    /// <summary>
    /// Defines an interface for translatable strings, providing methods to access context, message, and a frozen representation.
    /// </summary>
    class ITranslatableString {
    public:
        /// <summary>
        /// Returns a pointer to a string describing the context of the object
        /// in terms of the QTranslator.
        /// </summary>
        /// <returns>A pointer to a constant character string representing the object's context.</returns>
        virtual const char* context() const noexcept = 0;

        /// <summary>
        /// Returns a constant character pointer to a message string.
        /// This is the message which gets translated on language change.
        /// </summary>
        /// <returns>A pointer to a constant null-terminated character string containing the message.</returns>
        virtual const char* message() const noexcept = 0;

        /// <summary>
        /// Creates a frozen (immutable) representation of the object.
        /// That is, a QString that will not change even if the language setting changes.
        /// </summary>
        /// <returns>A QString containing the frozen representation of the object.</returns>
        virtual QString freeze() const = 0;

        // This is an interface-like class, so all of its operations do nothing.
        ITranslatableString() = default;
        virtual ~ITranslatableString() noexcept = default;

        ITranslatableString(const ITranslatableString&) = default;
        ITranslatableString& operator=(const ITranslatableString&) = default;

        ITranslatableString(ITranslatableString&&) noexcept = default;
        ITranslatableString& operator=(ITranslatableString&&) noexcept = default;
    };
}

#endif