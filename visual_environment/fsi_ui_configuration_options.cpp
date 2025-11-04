#include <QtGlobal>
#include <QDebug>
#include <QInputDialog>
#include <QCoreApplication>
#include <QStringList>
#include <QDir>
#include <thread>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00 // Targeting Windows 10 or later
#include <windows.h>

#include "fsi_ui_configuration_options.h"
#include "fsi_tools_messages.h"

namespace Components::FSITools {
    std::optional<FSIToolsAdapter::TranslatorFlags> ConfigurationOptions::getTranslatorDebugFlag(QWidget* parent) noexcept {
        constexpr const char* includeDebugOption = "Include Debug";
        constexpr const char* noIncludeDebugOption = "No Debug Symbols";
        constexpr int defaultOption = 0;
        constexpr bool isEditable = false;

        QStringList options = { includeDebugOption, noIncludeDebugOption };
        bool successful = false;

        QString result = QInputDialog::getItem(
            parent,
            QCoreApplication::translate(g_Context, g_Messages[MessageKeys::g_DebugOptionDialogTitle]),
            QCoreApplication::translate(g_Context, g_Messages[MessageKeys::g_DebugOptionDialogMessage]),
            options,
            defaultOption,
            isEditable,
            &successful
        );

        if (successful) {
            if (result == includeDebugOption) {
                return FSIToolsAdapter::TranslatorFlags::includeDebug;
            }

            if (result == noIncludeDebugOption) {
                return FSIToolsAdapter::TranslatorFlags::noIncludeDebug;
            }
        }

        return {};
    }

    std::optional<QString> ConfigurationOptions::getExecutionEnvironmentConfiguration(QWidget* parent) {
        constexpr const char* configurationFileFilter = "*.mods";
        constexpr int defaultOption = 0;
        constexpr bool isEditable = false;

        QDir applicationDirectory(QCoreApplication::applicationDirPath());
        QStringList foundConfigurations = applicationDirectory.entryList(
            { configurationFileFilter },
            QDir::Files | QDir::NoDotAndDotDot
        );

        for (QString& configurationFilePath : foundConfigurations) {
            configurationFilePath = applicationDirectory.filePath(
                configurationFilePath
            );
        }

        bool successful = false;
        QString result = QInputDialog::getItem(
            parent,
            QCoreApplication::translate(g_Context, g_Messages[MessageKeys::g_EEConfigurationFileDialogTitle]),
            QCoreApplication::translate(g_Context, g_Messages[MessageKeys::g_EEConfigurationFileDialogMessage]),
            foundConfigurations,
            defaultOption,
            isEditable,
            &successful
        );

        if (successful) {
            return result;
        }

        return {};
    }

    int ConfigurationOptions::getPreferredNumberOfExecutors() noexcept {
        try {
            DWORD bufferLength = 0;

            // First call to get required buffer size
            BOOL ok = GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &bufferLength);
            if (!ok && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
                throw std::runtime_error("GetLogicalProcessorInformationEx failed to get buffer size.");
            }

            std::unique_ptr<BYTE[]> buffer(new BYTE[bufferLength]);
            PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX logicalProcessorInformationBuffer = 
                reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.get());

            ok = GetLogicalProcessorInformationEx(RelationProcessorCore, logicalProcessorInformationBuffer, &bufferLength);
            if (!ok) {
                throw std::runtime_error("GetLogicalProcessorInformationEx failed.");
            }

            int coreCount = 0;
            BYTE* iterator = buffer.get();
            DWORD remaining = bufferLength;
            while (remaining > 0) {
                PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX logicalProcessorInfo = 
                    reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(iterator);

                if (logicalProcessorInfo->Relationship == RelationProcessorCore) {
                    coreCount++;
                }

                DWORD blockSize = logicalProcessorInfo->Size;
                if (blockSize > remaining) {
                    // This should not happen. However, I don't really trust ->Size field.
                    // It does not match sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX) in my tests.
                    qWarning() << "Logical processor information block size exceeds remaining buffer size.";
                    break;
                }

                iterator += blockSize;
                remaining -= blockSize;
            }

            if (coreCount < 1)
                throw std::runtime_error("No processor cores found.");

            return coreCount;
        }
        catch (const std::exception& exception) {
            qWarning() << "Error retrieving processor information:" << exception.what();
            if (std::thread::hardware_concurrency() > 0)
                return static_cast<int>(std::thread::hardware_concurrency());

            return 1;
        }
    }
}