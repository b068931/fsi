#ifndef APPLICATION_STYLES_MANAGER_H
#define APPLICATION_STYLES_MANAGER_H

#include <QObject>
#include <QString>

namespace Components::ApplicationStyle {
    /// <summary>
    /// Presents an interface to manage application styles (themes).
    /// Use Style enum to select between Light, Dark, or NoStyle.
    /// </summary>
    class ApplicationStylesManager : public QObject {
        Q_OBJECT

    public:
        enum class Style : int {
            Light = 0,
            Dark = 1,
            NoStyle = 2
        };

        explicit ApplicationStylesManager(Style preselectedStyle, QObject* parent = nullptr);
        ~ApplicationStylesManager() noexcept override;

        ApplicationStylesManager(const ApplicationStylesManager&) = delete;
        ApplicationStylesManager& operator= (const ApplicationStylesManager&) = delete;

        ApplicationStylesManager(ApplicationStylesManager&&) = delete;
        ApplicationStylesManager& operator= (ApplicationStylesManager&&) = delete;

        bool setStyle(Style newStyle);

    private:
        Style currentStyle;
        static bool loadStyleFromFile(const QString& filePath);
    };
}

#endif