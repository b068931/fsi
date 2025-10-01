#ifndef ITRANSLATABLE_STRING_INTERNATIONALIZATION_COMPONENT_H
#define ITRANSLATABLE_STRING_INTERNATIONALIZATION_COMPONENT_H

namespace Components::Internationalization {
    class ITranslatableString {
    public:
        /// <summary>
        /// Requests a retranslation operation. This is a pure virtual function that must be implemented by derived classes.
        /// Retranslation typically involves updating the displayed text to match the current language setting.
        /// </summary>
        virtual void requestRetranslation() = 0;
        virtual ~ITranslatableString() = default;
    };
}

#endif