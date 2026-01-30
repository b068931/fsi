#ifndef QSTRING_WRAPPER_INTERNATIONALIZATION_COMPONENT_H
#define QSTRING_WRAPPER_INTERNATIONALIZATION_COMPONENT_H

#include <memory>
#include "translatable_string_interface.h"

namespace Components::Internationalization {
    class QStringWrapper final : public ITranslatableString {
    public:
        /// <summary>
        /// Constructs a QStringWrapper object from a given QString.
        /// </summary>
        /// <param name="str">The QString to be wrapped.</param>
        explicit QStringWrapper(QString str);
        ~QStringWrapper() noexcept override;

        QStringWrapper(const QStringWrapper&);
        QStringWrapper& operator=(const QStringWrapper&);

        QStringWrapper(QStringWrapper&&) noexcept;
        QStringWrapper& operator=(QStringWrapper&&) noexcept;

        /// <summary>
        /// Wraps a QString message in a unique pointer to an ITranslatableString.
        /// This allows the QString to be used in contexts where an ITranslatableString is required.
        /// </summary>
        /// <param name="message">The QString message to be wrapped.</param>
        /// <returns>A std::unique_ptr to an ITranslatableString containing the provided message.</returns>
        static std::unique_ptr<ITranslatableString> wrap(QString message);

        /// <summary>
        /// Retrieves the context information associated with the object.
        /// For this object, it returns NULL as there is no specific context.
        /// </summary>
        /// <returns>A pointer to a null-terminated string containing the context information.</returns>
        const char* context() const noexcept override;

        /// <summary>
        /// For this object, this function returns NULL.
        /// </summary>
        /// <returns>NULL pointer.</returns>
        const char* message() const noexcept override;

        /// <summary>
        /// Returns the copy of the internal QString.
        /// </summary>
        /// <returns>A copy of the internal QString.</returns>
        QString freeze() const override;

    private:
        QString string;
    };
}

#endif