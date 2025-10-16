#include <QProcess>
#include "fsi_tools_adapter.h"

namespace Components::FSITools {
    void FSIToolsAdapter::onTranslatorFinished(int exitCode, QProcess::ExitStatus status) noexcept {
        ChildResult result = ChildResult::unknownError;
        switch (status) {
        case QProcess::NormalExit:
            result = ChildResult::terminated;
            break;
        case QProcess::CrashExit:
            result = ChildResult::crashed;
            break;
        default:
            result = ChildResult::unknownError;
            break;
        }

        emit this->translationResult(exitCode, result);
    }

    void FSIToolsAdapter::onTranslatorErrorOccurred(QProcess::ProcessError error) noexcept {
        if (error == QProcess::FailedToStart) {
            emit this->translationResult(DefaultReturnCode, ChildResult::failedToStart);
        }
    }

    void FSIToolsAdapter::onExecutionEnvironmentFinished(int exitCode, QProcess::ExitStatus status) noexcept {
        ChildResult result = ChildResult::unknownError;
        switch (status) {
        case QProcess::NormalExit:
            result = ChildResult::terminated;
            break;
        case QProcess::CrashExit:
            result = ChildResult::crashed;
            break;
        default:
            result = ChildResult::unknownError;
            break;
        }

        // Flush and close the log handle after the process ends
        if (this->executionEnvironmentLogFile != NULL && this->executionEnvironmentLogFile != INVALID_HANDLE_VALUE) {
            FlushFileBuffers(this->executionEnvironmentLogFile);
            CloseHandle(this->executionEnvironmentLogFile);
            this->executionEnvironmentLogFile = INVALID_HANDLE_VALUE;
        }

        emit this->executionEnvironmentResult(exitCode, result);
    }

    void FSIToolsAdapter::onExecutionEnvironmentErrorOccurred(QProcess::ProcessError error) noexcept {
        if (error == QProcess::FailedToStart) {
            if (this->executionEnvironmentLogFile != NULL && this->executionEnvironmentLogFile != INVALID_HANDLE_VALUE) {
                FlushFileBuffers(this->executionEnvironmentLogFile);
                CloseHandle(this->executionEnvironmentLogFile);
                this->executionEnvironmentLogFile = INVALID_HANDLE_VALUE;
            }

            emit this->executionEnvironmentResult(DefaultReturnCode, ChildResult::failedToStart);
        }
    }
}
