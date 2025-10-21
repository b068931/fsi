#include <QTextStream>
#include <QFile>
#include <array>
#include <QApplication>
#include "application_styles_manager.h"

constexpr const char* g_AvailableStylesMappings[] = {
    ":/styles/light_theme.qss",
    ":/styles/dark_theme.qss"
};

namespace Components::ApplicationStyle {
    ApplicationStylesManager::ApplicationStylesManager(Style preselectedStyle, QObject* parent)
        : QObject(parent), currentStyle(Style::NoStyle)
    {
        this->setStyle(preselectedStyle);
    }

    ApplicationStylesManager::~ApplicationStylesManager() noexcept = default;

    bool ApplicationStylesManager::setStyle(Style newStyle) {
        if (newStyle == Style::NoStyle) {
            qApp->setStyleSheet(QString());

            this->currentStyle = newStyle;
            return true;
        }

        if (this->currentStyle != newStyle) {
            int newStyleIndex = static_cast<int>(newStyle);
            if (newStyleIndex >= 0 && std::isless(newStyleIndex, std::size(g_AvailableStylesMappings))) {
                const QString styleFilePath = g_AvailableStylesMappings[static_cast<int>(newStyle)];
                if (ApplicationStylesManager::loadStyleFromFile(styleFilePath)) {
                    this->currentStyle = newStyle;
                    return true;
                }

                return false;
            }

            qWarning() << "ApplicationStylesManager::setStyle: Invalid style index requested.";
            return false;
        }

        return true;
    }

    bool ApplicationStylesManager::loadStyleFromFile(const QString& filePath) {
        QFile styleFile(filePath);
        if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream styleStream(&styleFile);
            QString newStyle = styleStream.readAll();

            if (styleFile.error() == QFile::NoError) {
                qApp->setStyleSheet(newStyle);
                styleFile.close();
                return true;
            }

            qWarning() << "ApplicationStylesManager::loadStyleFromFile: Error reading style file:" << filePath;
            styleFile.close();
        }
        else {
            qWarning() << "ApplicationStylesManager::loadStyleFromFile: Unable to open style file:" << filePath;
        }

        return false;
    }

}