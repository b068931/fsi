#ifndef STATIC_TRANSLATABLE_STRING_INTERNATIONALIZATION_COMPONENT_H
#define STATIC_TRANSLATABLE_STRING_INTERNATIONALIZATION_COMPONENT_H

#include <QString>

#include "translatable_string_interface.h"

namespace Components::Internationalization {
    /// <summary>
    /// Represents a translatable string with a static context and message.
    /// If you want to change the message for this string, you must recreate it.
    /// </summary>
    class StaticTranslatableString : public ITranslatableString {
    public:
        /// <summary>
        /// Deletes the default constructor for the StaticTranslatableString class, preventing its instantiation without arguments.
        /// </summary>
        StaticTranslatableString() = delete;
        ~StaticTranslatableString() noexcept override;

        /// <summary>
        /// Constructs a StaticTranslatableString with the specified context and message.
        /// </summary>
        /// <param name="context">A pointer to a null-terminated string representing the translation context. Must not be null.</param>
        /// <param name="message">A pointer to a null-terminated string representing the message to be translated. Must not be null.</param>
        StaticTranslatableString(const char* context, const char* message);

        StaticTranslatableString(const StaticTranslatableString& other);
        StaticTranslatableString& operator=(const StaticTranslatableString& other);

        StaticTranslatableString(StaticTranslatableString&& other) noexcept = delete;
        StaticTranslatableString& operator=(StaticTranslatableString&& other) noexcept = delete;

        /// <summary>
        /// Wraps a message and its context into a translatable string object.
        /// </summary>
        /// <param name="context">A string representing the context for the message, as expected by the QTranslator.</param>
        /// <param name="message">The message string to be wrapped and made translatable.</param>
        /// <returns>A unique pointer to an ITranslatableString object containing the provided context and message.</returns>
        static std::unique_ptr<ITranslatableString> wrap(const char* context, const char* message);

        /// <summary>
        /// Returns a pointer to a null-terminated string representing the context information.
        /// </summary>
        /// <returns>A pointer to a constant null-terminated string containing the context information.</returns>
        virtual const char* context() const noexcept override;

        /// <summary>
        /// Returns a pointer to a null-terminated string with an original message that can be translated.
        /// </summary>
        /// <returns>A pointer to a null-terminated character string with the original message.</returns>
        virtual const char* message() const noexcept override;

        /// <summary>
        /// Returns a string representation of the object's frozen state.
        /// That is, a QString that will not change even if the language setting changes.
        /// </summary>
        /// <returns>A QString representing the frozen message of the object.</returns>
        virtual QString freeze() const override;

    private:
        const char* savedContext;
        const char* savedMessage;
    };
}

#endif