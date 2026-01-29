#include <QTextStream>
#include <QFile>
#include <QSet>
#include <QDirIterator>
#include <QApplication>
#include <QFontDatabase>
#include <QStringList>
#include <array>

#include "application_styles_manager.h"

constexpr const char* g_AvailableStylesMappings[] = {
    ":/styles/light_theme.qss",
    ":/styles/dark_theme.qss"
};

namespace Components::ApplicationStyle {
    ApplicationStylesManager::ApplicationStylesManager(
        Style preselectedStyle,
        const QString& fontsRoot,
        QObject* parent
    )
        : QObject(parent), currentStyle(Style::NoStyle)
    {
        loadCustomFonts(fontsRoot);
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
                if (loadStyleFromFile(styleFilePath)) {
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

    void ApplicationStylesManager::loadCustomFonts(const QString& fontsRoot) {
        if (fontsRoot.isEmpty()) {
            qDebug() << "ApplicationStylesManager::loadCustomFonts: No fonts root directory specified.";
            return;
        }

        QDirIterator fontsIterator{ fontsRoot, QDirIterator::Subdirectories };
        QSet<QString> loadedFontFamilies;

        while (fontsIterator.hasNext()) {
            QFile fontFile{ fontsIterator.next() };
            if (fontFile.open(QFile::ReadOnly)) {
                int fontId = QFontDatabase::addApplicationFontFromData(fontFile.readAll());
                if (fontId != -1) {
                    QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);
                    for (const QString& family : fontFamilies) {
                        loadedFontFamilies.insert(family);
                    }
                }
                else {
                    qWarning() << "ApplicationStylesManager::loadCustomFonts: Failed to load font from file:" << fontFile.fileName();
                }

                fontFile.close();
            }
            else {
                qWarning() << "ApplicationStylesManager::loadCustomFonts: Unable to open font file:" << fontFile.fileName();
            }
        }

        if (loadedFontFamilies.empty()) {
            qDebug() << "No custom fonts were loaded from the specified directory:" << fontsRoot;
        }
        else {
            QStringList fontFamilies;
            for (const QString& fontFamily : loadedFontFamilies) {
                fontFamilies += fontFamily;
            }

            qDebug() << "Loaded custom fonts:" << fontFamilies.join(", ");
        }
    }
}
