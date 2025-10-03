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

        static std::unique_ptr<ITranslatableString> wrap(const char* context, const char* message);

        virtual const char* context() const noexcept override;
        virtual const char* message() const noexcept override;
        virtual QString freeze() const override;

        ~StaticTranslatableString() noexcept override;

    private:
        const char* savedContext;
        const char* savedMessage;
    };
}

#endif