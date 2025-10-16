#ifndef FSI_UI_CONFIGURATION_OPTIONS_H
#define FSI_UI_CONFIGURATION_OPTIONS_H

#include <QString>
#include "fsi_tools_adapter.h"

namespace Components::FSITools {
    /// <summary>
    /// A helper class which contains all user interaction logic required to obtain
    /// configuration options for FSIToolsAdapter operations. Also contains methods
    /// which query system for preferable options.
    /// </summary>
    class ConfigurationOptions final {
    public:
        /// <summary>
        /// Asks the user to choose the translator debug flag. Required to be called from the main (GUI) thread.
        /// </summary>
        /// <returns>A user-chosen option.</returns>
        static FSIToolsAdapter::TranslatorFlags getTranslatorDebugFlag() noexcept;

        /// <summary>
        /// Retrieves the execution environment configuration from the user as a QString.
        /// </summary>
        /// <returns>A QString containing the path to the execution environment configuration.</returns>
        static QString getExecutionEnvironmentConfiguration();

        /// <summary>
        /// Returns the preferred number of executors for the current environment.
        /// Queries the number of physical CPU cores and returns that value.
        /// </summary>
        /// <returns>An integer indicating the preferred number of executor threads or workers.</returns>
        static int getPreferredNumberOfExecutors() noexcept;
    };
}

#endif