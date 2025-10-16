#include <QtGlobal>
#include <QDebug>
#include <thread>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00 // Targeting Windows 10 or later
#include <windows.h>

#include "fsi_ui_configuration_options.h"

namespace Components::FSITools {
    FSIToolsAdapter::TranslatorFlags ConfigurationOptions::getTranslatorDebugFlag() noexcept {
    }

    QString ConfigurationOptions::getExecutionEnvironmentConfiguration() {
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
            while (remaining >= sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)) {
                PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX logicalProcessorInfo = 
                    reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(iterator);

                if (logicalProcessorInfo->Relationship == RelationProcessorCore) {
                    coreCount++;
                }

                DWORD blockSize = logicalProcessorInfo->Size;
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