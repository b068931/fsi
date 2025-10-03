#include <QCoreApplication>
#include <QtGlobal>
#include <memory>

#include "static_translatable_string.h"

namespace Components::Internationalization {
    StaticTranslatableString::StaticTranslatableString(const char* context, const char* message)
        : savedContext(context), savedMessage(message) {
        Q_ASSERT(context != nullptr && message != nullptr && "Context and message must not be null.");
    }

    StaticTranslatableString::StaticTranslatableString(const StaticTranslatableString& other)
        : savedContext(other.savedContext), savedMessage(other.savedMessage) {
    }

    StaticTranslatableString& StaticTranslatableString::operator=(const StaticTranslatableString& other) {
        if (this != &other) {
            this->savedContext = other.savedContext;
            this->savedMessage = other.savedMessage;
        }
        return *this;
    }

    std::unique_ptr<ITranslatableString> StaticTranslatableString::wrap(const char* context, const char* message) {
        return std::make_unique<StaticTranslatableString>(context, message);
    }

    const char* StaticTranslatableString::context() const noexcept {
        return this->savedContext;
    }

    const char* StaticTranslatableString::message() const noexcept {
        return this->savedMessage;
    }

    QString StaticTranslatableString::freeze() const {
        return QCoreApplication::translate(this->savedContext, this->savedMessage);
    }

    StaticTranslatableString::~StaticTranslatableString() noexcept = default;
}
