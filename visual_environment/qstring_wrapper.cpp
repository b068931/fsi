#include "qstring_wrapper.h"

#include <utility>

namespace Components::Internationalization {
    QStringWrapper::QStringWrapper(QString str)
        : string(std::move(str)) {}

    QStringWrapper::QStringWrapper(const QStringWrapper&) = default;
    QStringWrapper& QStringWrapper::operator=(const QStringWrapper&) = default;

    QStringWrapper::QStringWrapper(QStringWrapper&&) noexcept = default;
    QStringWrapper& QStringWrapper::operator=(QStringWrapper&&) noexcept = default;

    QStringWrapper::~QStringWrapper() noexcept = default;

    std::unique_ptr<ITranslatableString> QStringWrapper::wrap(QString message) {
        return std::make_unique<QStringWrapper>(std::move(message));
    }

    const char* QStringWrapper::context() const noexcept {
        return nullptr;
    }

    const char* QStringWrapper::message() const noexcept {
        return nullptr;
    }

    QString QStringWrapper::freeze() const {
        return this->string;
    }
}
