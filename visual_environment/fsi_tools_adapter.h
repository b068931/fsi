#ifndef FSI_TOOLS_ADAPTER_H
#define FSI_TOOLS_ADAPTER_H

// While I am using QProcess, I still need to do some windows-specific configuration.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <QObject>
#include <QString>
#include <QProcess>

namespace Components::FSITools {
    /// <summary>
    /// Provides a convenient interface for working with FSI tools.
    /// Automatically manages child processes for translation and execution environment.
    /// Properly configures all IO redirections for child processes.
    /// </summary>
    class FSIToolsAdapter final : public QObject {
        Q_OBJECT

    public:
        enum class ChildResult {
            terminated = 0,
            failedToStart = 1,
            crashed = 2,
            unknownError = 3
        };

        enum class TranslatorFlags {
            includeDebug,
            noIncludeDebug
        };

        explicit FSIToolsAdapter(QObject* parent = nullptr);
        ~FSIToolsAdapter() noexcept override;

        FSIToolsAdapter(const FSIToolsAdapter&) = delete;
        FSIToolsAdapter& operator=(const FSIToolsAdapter&) = delete;

        FSIToolsAdapter(FSIToolsAdapter&&) = delete;
        FSIToolsAdapter& operator=(FSIToolsAdapter&&) = delete;

        /// <summary>
        /// Starts the translation process for a program (*.fsi, *.tfsi) and outputs the result to a specified path (*.bfsi).
        /// Will fail if the translator process is already running.
        /// </summary>
        /// <param name="programPath">The file path to the program to be translated.</param>
        /// <param name="outputPath">The file path where the translation output will be saved.</param>
        /// <param name="debugFlag">Flags that control debugging options for the translation process.</param>
        /// <returns>true if the translation process starts successfully; otherwise, false.</returns>
        bool startProgramTranslation(
            const QString& programPath, 
            const QString& outputPath,
            TranslatorFlags debugFlag
        );

        /// <summary>
        /// Starts the execution environment for a program using the specified configuration (*.mods file) and number of executors.
        /// The number of executors must be at least 1. Will fail if the execution environment process is already running.
        /// Executors correspond to the number of system threads that will be used during execution.
        /// </summary>
        /// <param name="translatedProgramPath">The path to the translated program to be executed (*.bfsi).</param>
        /// <param name="configurationPath">The path to the configuration file for the execution environment.</param>
        /// <param name="executorsCount">The number of executor instances to launch.</param>
        /// <param name="logFilePath">The path to the log file.</param>
        /// <returns>True if the execution environment was started successfully; otherwise, false.</returns>
        bool startExecutionEnvironment(
            const QString& configurationPath,
            int executorsCount,
            const QString& translatedProgramPath,
            const QString& logFilePath
        );

        /// <summary>
        /// Retrieves the most recent translation target.
        /// </summary>
        /// <returns>A QString representing the path to the last translation target.</returns>
        QString getLastTranslationTarget() const;

        /// <summary>
        /// Retrieves the result of the most recent translation operation.
        /// </summary>
        /// <returns>A QString containing the path to the result of the last translation.</returns>
        QString getLastTranslationResult() const;

        /// <summary>
        /// Retrieves the program associated with the last execution environment.
        /// </summary>
        /// <returns>A QString containing the path of the last program that ran in the execution environment.</returns>
        QString getLastExecutionEnvironmentProgram() const;

        /// <summary>
        /// Retrieves the configuration of the last execution environment.
        /// </summary>
        /// <returns>A QString containing the path to the configuration file which was used to run the execution environment.</returns>
        QString getLastExecutionEnvironmentConfiguration() const;

    signals:
        /// <summary>
        /// Emitted when the translation process starts.
        /// </summary>
        void translatorStarted();

        /// <summary>
        /// Notifies about the result of a translation operation, including its exit code and result information.
        /// </summary>
        /// <param name="exitCode">The exit code returned by the translation process, typically indicating success or failure.</param>
        /// <param name="result">Describes whether the process finished, failed to start or crashed.</param>
        void translationResult(int exitCode, ChildResult result);

        /// <summary>
        /// Emitted when the execution environment process starts.
        /// </summary>
        void executionEnvironmentStarted();

        /// <summary>
        /// Notifies about the result of a child process execution environment.
        /// </summary>
        /// <param name="exitCode">The exit code returned by the child process.</param>
        /// <param name="result">Describes whether the process finished, failed to start or crashed.</param>
        void executionEnvironmentResult(int exitCode, ChildResult result);

    private slots:
        void onTranslatorFinished(int exitCode, QProcess::ExitStatus status);
        void onTranslatorErrorOccurred(QProcess::ProcessError error);

        void onExecutionEnvironmentFinished(int exitCode, QProcess::ExitStatus status);
        void onExecutionEnvironmentErrorOccurred(QProcess::ProcessError error);

    private:
        QProcess* translator;
        QProcess* executionEnvironment;
        HANDLE executionEnvironmentLogFile;

        QString lastTranslationTarget;
        QString lastTranslationResult;
        QString lastExecutionEnvironmentProgram;
        QString lastExecutionEnvironmentConfiguration;

        void setUpChildren();
    };
}

#endif