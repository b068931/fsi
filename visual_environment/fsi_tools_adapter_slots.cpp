#include <QProcess>
#include <QCoreApplication>
#include <QDebug>

#include "fsi_tools_adapter.h"
#include "fsi_tools_messages.h"

namespace Components::FSITools {
    void FSIToolsAdapter::onTranslatorFinished(int exitCode, QProcess::ExitStatus status) noexcept {
        constexpr int NTSTATUS_CTRL_C_EXIT = -1073741510;

        ChildResult result = ChildResult::unknownError;
        if (exitCode == NTSTATUS_CTRL_C_EXIT) {
            result = ChildResult::killedByUser;
        }
        else {
            switch (status) {
            case QProcess::NormalExit: {
                result = ChildResult::terminated;
                break;
            }
            case QProcess::CrashExit: {
                result = ChildResult::crashed;
                break;
            }
            default: {
                result = ChildResult::unknownError;
                break;
            }
            }
        }

        emit this->translationResult(exitCode, result);
    }

    void FSIToolsAdapter::onTranslatorErrorOccurred(QProcess::ProcessError error) noexcept {
        if (error == QProcess::FailedToStart) {
            emit this->translationResult(DefaultReturnCode, ChildResult::failedToStart);
        }
        else {
            qDebug() << "Translator process error occurred:" << static_cast<int>(error);
        }
    }

    void FSIToolsAdapter::onExecutionEnvironmentFinished(int exitCode, QProcess::ExitStatus status) noexcept {
        constexpr int NTSTATUS_CTRL_C_EXIT = -1073741510;
        
        ChildResult result = ChildResult::unknownError;
        if (exitCode == NTSTATUS_CTRL_C_EXIT) {
            result = ChildResult::killedByUser;
        }
        else {
            switch (status) {
            case QProcess::NormalExit: {
                result = ChildResult::terminated;
                break;
            }
            case QProcess::CrashExit: {
                result = ChildResult::crashed;
                break;
            }
            default: {
                result = ChildResult::unknownError;
                break;
            }
            }
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
        else {
            qDebug() << "Execution environment process error occurred:" << static_cast<int>(error);
        }
    }

    void FSIToolsAdapter::onRetranslateUI() {
        // Must use "translate" explicitly with context because messages
        // are not tied to the context of the FSIToolsAdapter class.

        this->translatorWindowTitle =
            QCoreApplication::translate(
                g_Context,
                g_Messages[g_TranslatorConsoleWindowTitle]
            ).toStdWString();

        this->executionEnvironmentWindowTitle =
            QCoreApplication::translate(
                g_Context,
                g_Messages[g_EEConsoleWindowTitle]
            ).toStdWString();
    }
}
