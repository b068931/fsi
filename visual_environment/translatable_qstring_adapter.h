#ifndef TRANSLATABLE_QSTRING_ADAPTER_INTERNATIONALIZATION_COMPONENT_H
#define TRANSLATABLE_QSTRING_ADAPTER_INTERNATIONALIZATION_COMPONENT_H

#include <QString>
#include <cstdlib>

#include "translatable_string_interface.h"

namespace Components::Internationalization {
    /// <summary>
    /// TranslatableQStringAdapter is a concrete implementation of the ITranslatableString interface.
    /// It encapsulates a QString and provides functionality to update its value based on the current language setting.
    /// </summary>
    class TranslatableQStringAdapter : public QString, public ITranslatableString {
    private:
        const char* context;
        const char* message;

    public:
        TranslatableQStringAdapter();
        explicit TranslatableQStringAdapter(const char* context, const char* message);

        TranslatableQStringAdapter(const TranslatableQStringAdapter& other);
        TranslatableQStringAdapter& operator=(const TranslatableQStringAdapter& other);

        TranslatableQStringAdapter(TranslatableQStringAdapter&& other) noexcept;
        TranslatableQStringAdapter& operator=(TranslatableQStringAdapter&& other) noexcept;

        virtual void requestRetranslation() override;
    };
}
#endif