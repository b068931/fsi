#ifndef QSTRING_WRAPPER_INTERNATIONALIZATION_COMPONENT_H
#define QSTRING_WRAPPER_INTERNATIONALIZATION_COMPONENT_H

#include <memory>
#include "translatable_string_interface.h"

namespace Components::Internationalization {
    class QStringWrapper final : public ITranslatableString {
    public:
        explicit QStringWrapper(QString str);

        QStringWrapper(const QStringWrapper&);
        QStringWrapper& operator=(const QStringWrapper&);

        QStringWrapper(QStringWrapper&&) noexcept;
        QStringWrapper& operator=(QStringWrapper&&) noexcept;

        ~QStringWrapper() noexcept override;

        static std::unique_ptr<ITranslatableString> wrap(QString message);

        virtual const char* context() const noexcept override;
        virtual const char* message() const noexcept override;
        virtual QString freeze() const override;

    private:
        QString string;
    };
}

#endif