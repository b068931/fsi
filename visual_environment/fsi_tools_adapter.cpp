#include <QCoreApplication>
#include <QStringList>
#include <string>

#include "fsi_tools_adapter.h"

namespace Components::FSITools {
    FSIToolsAdapter::FSIToolsAdapter(QObject* parent)
        : QObject(parent), translator(nullptr), executionEnvironment(nullptr), executionEnvironmentLogFile(INVALID_HANDLE_VALUE)
    {
        this->setUpChildren();
    }

    FSIToolsAdapter::~FSIToolsAdapter() noexcept {
        if (this->translator) {
            // On Windows "Starting" should not be observable, as the processes are created synchronously.
            if (this->translator->state() == QProcess::Starting) {
                this->translator->waitForStarted();
            }

            if (this->translator->state() != QProcess::NotRunning) {
                this->translator->kill();
                this->translator->waitForFinished();
            }

            delete this->translator;
        }

        if (this->executionEnvironment) {
            if (this->executionEnvironment->state() == QProcess::Starting) {
                this->executionEnvironment->waitForStarted();
            }

            if (this->executionEnvironment->state() != QProcess::NotRunning) {
                this->executionEnvironment->kill();
                this->executionEnvironment->waitForFinished();
            }

            delete this->executionEnvironment;
        }

        if (this->executionEnvironmentLogFile != NULL && this->executionEnvironmentLogFile != INVALID_HANDLE_VALUE) {
            FlushFileBuffers(this->executionEnvironmentLogFile);
            CloseHandle(this->executionEnvironmentLogFile);
        }
    }

    void FSIToolsAdapter::startProgramTranslation(
            const QString& programPath,
            const QString& outputPath,
            TranslatorFlags debugFlag
        )
    {
        Q_ASSERT(this->translator && "Translator process has not been set up.");

        constexpr char executableName[]{ "translator-stub.bat" };
        constexpr int maximumWaitTime = 5000;

        if (this->translator->state() != QProcess::NotRunning) {
            emit this->translationResult(DefaultReturnCode, ChildResult::alreadyRunning);
        }

        // Save last known paths
        this->lastTranslationTarget = programPath;
        this->lastTranslationResult = outputPath;

        QStringList arguments;
        arguments << programPath
                  << outputPath
                  << (debugFlag == TranslatorFlags::includeDebug
                         ? QStringLiteral("include-debug")
                         : QStringLiteral("no-debug"));

        this->translator->start(executableName, arguments);
        this->translator->waitForStarted(maximumWaitTime);

        emit this->translatorStarted();
    }

    void FSIToolsAdapter::startExecutionEnvironment(
            const QString& configurationPath,
            int executorsCount,
            const QString& translatedProgramPath,
            const QString& logFilePath
        )
    {
        Q_ASSERT(this->executionEnvironment && "Execution environment process has not been set up.");

        constexpr char executableName[]{ "mediator-stub.bat" };
        constexpr int maximumWaitTime = 5000;

        if (this->executionEnvironment->state() != QProcess::NotRunning) {
            emit this->executionEnvironmentResult(DefaultReturnCode, ChildResult::alreadyRunning);
        }

        if (executorsCount < 1) {
            emit this->executionEnvironmentResult(DefaultReturnCode, ChildResult::failedToStart);
        }

        // Close any existing log handle and recreate it for the new start.
        if (this->executionEnvironmentLogFile != NULL && this->executionEnvironmentLogFile != INVALID_HANDLE_VALUE) {
            FlushFileBuffers(this->executionEnvironmentLogFile);
            CloseHandle(this->executionEnvironmentLogFile);
            this->executionEnvironmentLogFile = INVALID_HANDLE_VALUE;
        }

        SECURITY_ATTRIBUTES logFileSecurity{};
        logFileSecurity.nLength = sizeof(SECURITY_ATTRIBUTES);
        logFileSecurity.bInheritHandle = TRUE;
        logFileSecurity.lpSecurityDescriptor = nullptr;

        std::wstring nativeLogFilePath = logFilePath.toStdWString();
        HANDLE logHandle = CreateFileW(
            nativeLogFilePath.data(),
            GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            &logFileSecurity,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        this->executionEnvironmentLogFile = logHandle;
        if (this->executionEnvironmentLogFile == INVALID_HANDLE_VALUE) {
            // If we failed to create a log file, keep INVALID_HANDLE_VALUE.
            emit this->executionEnvironmentResult(DefaultReturnCode, ChildResult::failedToStart);
        }

        // Save last known program/configuration
        this->lastExecutionEnvironmentProgram = translatedProgramPath;
        this->lastExecutionEnvironmentConfiguration = configurationPath;

        QStringList arguments;
        arguments << configurationPath
                  << QString::number(executorsCount)
                  << translatedProgramPath;

        this->executionEnvironment->start(executableName, arguments);
        this->executionEnvironment->waitForStarted(maximumWaitTime);

        emit this->executionEnvironmentStarted();
    }

    QString FSIToolsAdapter::getLastTranslationTarget() const {
        return this->lastTranslationTarget;
    }

    QString FSIToolsAdapter::getLastTranslationResult() const {
        return this->lastTranslationResult;
    }

    QString FSIToolsAdapter::getLastExecutionEnvironmentProgram() const {
        return this->lastExecutionEnvironmentProgram;
    }

    QString FSIToolsAdapter::getLastExecutionEnvironmentConfiguration() const {
        return this->lastExecutionEnvironmentConfiguration;
    }

    void FSIToolsAdapter::setUpChildren() {
        this->translator = new QProcess(this);
        this->executionEnvironment = new QProcess(this);

        // For the translator, we are not going to capture anything that it outputs.
        // It will either succeed or fail, and we will check its exit code.
        // All interaction with the user will be done through its own console window.
        this->translator->setProcessChannelMode(QProcess::ForwardedChannels);
        this->translator->setWorkingDirectory(QCoreApplication::applicationDirPath());
        this->translator->setCreateProcessArgumentsModifier([this](QProcess::CreateProcessArguments* arguments) {
            constexpr DWORD GREY_BACKGROUND = BACKGROUND_INTENSITY;
            constexpr DWORD WHITE_FOREGROUND = FOREGROUND_INTENSITY
                | FOREGROUND_RED
                | FOREGROUND_GREEN
                | FOREGROUND_BLUE;

            arguments->inheritHandles = FALSE;
            arguments->startupInfo->lpTitle = this->translatorWindowTitle.data();

            arguments->flags |= CREATE_NEW_CONSOLE;
            arguments->startupInfo->dwFlags &= ~STARTF_USESTDHANDLES;

            arguments->startupInfo->dwFlags |= STARTF_USEFILLATTRIBUTE;
            arguments->startupInfo->dwFillAttribute |= GREY_BACKGROUND;
            arguments->startupInfo->dwFillAttribute |= WHITE_FOREGROUND;
        });

        // For the execution environment, we want to capture its errors.
        // This one is a bit tricky, because we want its console to handle standard output and standard input,
        // but we want to capture its standard error output to a log file. In order to implement that,
        // we'll have to rely on some very specific Windows API behavior.
        // From the https://learn.microsoft.com/en-us/windows/console/getstdhandle:
        /*
        * When attaching to a new console, standard handles are always replaced with console handles unless STARTF_USESTDHANDLES was specified during process creation.
        * If the existing value of the standard handle is NULL, or the existing value of the standard handle looks like a console pseudohandle, the handle is replaced with a console handle.
        * When a parent uses both CREATE_NEW_CONSOLE and STARTF_USESTDHANDLES to create a console process, standard handles will not be replaced unless the existing value of the standard handle is NULL or a console pseudohandle.]
        */
        // That is, for the stderr handle, we will create a log file and set the handle to that file.
        // While for stdin and stdout, we will use NULL, which will cause GetStdHandle to return the console handles.
        // This works because I know for sure that PRTS' stdio interaction logic uses GetStdHandle to retrieve the STD handles.
        this->executionEnvironment->setProcessChannelMode(QProcess::ForwardedOutputChannel);
        this->executionEnvironment->setWorkingDirectory(QCoreApplication::applicationDirPath());
        this->executionEnvironment->setCreateProcessArgumentsModifier([this](QProcess::CreateProcessArguments* arguments) {
            // TODO: Configure this so that the child inherits only the file handle for the log file.
            //       For now, we will just let it inherit everything.

            constexpr DWORD GREY_BACKGROUND = BACKGROUND_INTENSITY;
            constexpr DWORD WHITE_FOREGROUND = FOREGROUND_INTENSITY
                | FOREGROUND_RED
                | FOREGROUND_GREEN
                | FOREGROUND_BLUE;

            arguments->inheritHandles = TRUE;
            arguments->flags |= CREATE_NEW_CONSOLE;
            arguments->startupInfo->lpTitle = this->executionEnvironmentWindowTitle.data();

            arguments->startupInfo->dwFlags |= STARTF_USESTDHANDLES;
            arguments->startupInfo->hStdError = this->executionEnvironmentLogFile;
            arguments->startupInfo->hStdInput = NULL;  // Will be replaced with console handle.
            arguments->startupInfo->hStdOutput = NULL; // Will be replaced with console handle.

            arguments->startupInfo->dwFlags |= STARTF_USEFILLATTRIBUTE;
            arguments->startupInfo->dwFillAttribute |= GREY_BACKGROUND;
            arguments->startupInfo->dwFillAttribute |= WHITE_FOREGROUND;
        });

        // Now just connect the signals.
        connect(this->translator, &QProcess::finished, 
            this, &FSIToolsAdapter::onTranslatorFinished);

        connect(this->translator, &QProcess::errorOccurred, 
            this, &FSIToolsAdapter::onTranslatorErrorOccurred);

        connect(this->executionEnvironment, &QProcess::finished, 
                this, &FSIToolsAdapter::onExecutionEnvironmentFinished);

        connect(this->executionEnvironment, &QProcess::errorOccurred, 
            this, &FSIToolsAdapter::onExecutionEnvironmentErrorOccurred);
    }
}