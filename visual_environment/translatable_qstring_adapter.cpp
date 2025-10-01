#include <QCoreApplication>
#include <utility>
#include "translatable_qstring_adapter.h"

namespace Components::Internationalization {
    TranslatableQStringAdapter::TranslatableQStringAdapter()
        : context(nullptr), message(nullptr) {}

    TranslatableQStringAdapter::TranslatableQStringAdapter(const char* context, const char* message)
        : context(context), message(message) {
        Q_ASSERT(context != nullptr && "You are expected to assign a text context.");
        Q_ASSERT(message != nullptr && "You are expected to assign a text message.");

        this->QString::operator=(
            QCoreApplication::translate(context, message)
        );
    }

    TranslatableQStringAdapter::TranslatableQStringAdapter(const TranslatableQStringAdapter& other)
        : QString(other), context(other.context), message(other.message) {}

    TranslatableQStringAdapter& TranslatableQStringAdapter::operator=(const TranslatableQStringAdapter& other) {
        if (this != &other) {
            this->context = other.context;
            this->message = other.message;
        }

        // Moved out of the if block to ensure that we don't impact QString's internal logic.
        this->QString::operator=(other);
        return *this;
    }

    TranslatableQStringAdapter::TranslatableQStringAdapter(TranslatableQStringAdapter&& other) noexcept
        : QString(static_cast<QString&&>(std::move(other))),
          context(other.context),
          message(other.message)
    {
        other.context = nullptr;
        other.message = nullptr;
    }

    TranslatableQStringAdapter& TranslatableQStringAdapter::operator=(TranslatableQStringAdapter&& other) noexcept {
        if (this != &other) {
            this->context = other.context;
            this->message = other.message;

            other.context = nullptr;
            other.message = nullptr;
        }

        // Moved out of the if block to ensure that we don't impact QString's internal logic.
        this->QString::operator=(std::move(other));
        return *this;
    }

    void TranslatableQStringAdapter::requestRetranslation() {
        if (context != nullptr) {
            this->QString::operator=(
                QCoreApplication::translate(context, message)
            );
        }
    }
}
