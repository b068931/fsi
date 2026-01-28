#ifndef APPLICATION_STYLES_MANAGER_H
#define APPLICATION_STYLES_MANAGER_H

#include <QObject>
#include <QString>

namespace Components::ApplicationStyle {
    /// <summary>
    /// Presents an interface to manage application styles (themes).
    /// Use Style enum to select between Light, Dark, or NoStyle.
    /// Instantiating this class is also required to load custom fonts used by the application.
    /// </summary>
    class ApplicationStylesManager : public QObject {
        Q_OBJECT

    public:
        enum class Style : int {
            Light = 0,
            Dark = 1,
            NoStyle = 2
        };

        /// <summary>
        /// Constructs an ApplicationStylesManager with the specified preselected style and fonts root directory.
        /// This class will recursively load all fonts located in the specified fonts root directory.
        /// </summary>
        /// <param name="preselectedStyle">The style to be preselected upon initialization.</param>
        /// <param name="fontsRoot">The root directory path where fonts are located. Use empty string to specify no directory.</param>
        /// <param name="parent">The parent QObject. Defaults to nullptr if not specified.</param>
        explicit ApplicationStylesManager(
            Style preselectedStyle,
            const QString& fontsRoot,
            QObject* parent = nullptr
        );

        ~ApplicationStylesManager() noexcept override;

        ApplicationStylesManager(const ApplicationStylesManager&) = delete;
        ApplicationStylesManager& operator= (const ApplicationStylesManager&) = delete;

        ApplicationStylesManager(ApplicationStylesManager&&) = delete;
        ApplicationStylesManager& operator= (ApplicationStylesManager&&) = delete;

        /// <summary>
        /// Attempts to load and apply the specified style.
        /// </summary>
        /// <param name="newStyle">The style chosen from the enumeration.</param>
        /// <returns>Specifies whether the operation was successful.</returns>
        bool setStyle(Style newStyle);

    private:
        Style currentStyle;

        static bool loadStyleFromFile(const QString& filePath);
        static void loadCustomFonts(const QString& fontsRoot);
    };
}

#endif