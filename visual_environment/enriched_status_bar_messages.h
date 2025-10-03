#ifndef ENRICHED_STATUS_BAR_MESSAGES_H
#define ENRICHED_STATUS_BAR_MESSAGES_H

#include <QTTranslation>

constexpr const char* g_Context = "CustomWidgets::EnrichedStatusBar";
constexpr const char* g_Messages[] = {
    //: Label in the status bar showing tooltips.
    QT_TRANSLATE_NOOP("CustomWidgets::EnrichedStatusBar", "Tooltips"),

    //: Label in the status bar showing the working directory of the application.
    QT_TRANSLATE_NOOP("CustomWidgets::EnrichedStatusBar", "Working Directory"),

    //: Label in the status bar showing the result of translator execution.
    QT_TRANSLATE_NOOP("CustomWidgets::EnrichedStatusBar", "Execution Environment State"),

    //: Label in the status bar showing the state of the execution environment.
    QT_TRANSLATE_NOOP("CustomWidgets::EnrichedStatusBar", "Translator Result")
};

enum MessageKeys {
    g_TooltipTooltips = 0,
    g_TooltipWorkingDirectory = 1,
    g_TooltipExecutionEnvironmentState = 2,
    g_TooltipTranslatorResult = 3
};

#endif