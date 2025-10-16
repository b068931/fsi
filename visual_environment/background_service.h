#ifndef BACKGROUND_SERVICE_UTILITY_H
#define BACKGROUND_SERVICE_UTILITY_H

#include <QObject>
#include <QMetaObject>
#include <QThread>
#include <QScopedPointer>

#include <type_traits>
#include <functional>

namespace Utility {
    /// <summary>
    /// A container class for the QObject that should be moved to a separate thread and
    /// run there for a long time. That is, a service. Relies on Qt's signals/slots system to ensure
    /// that interactions with the object are safe by using queued connections.
    /// </summary>
    /// <typeparam name="ContainedQObjectType">A type to contain. Must be derived from QObject.</typeparam>
    template<typename ContainedQObjectType>
        requires std::is_base_of_v<QObject, ContainedQObjectType>
    class BackgroundService final {
    public:
        /// <summary>
        /// Constructs a new BackgroundService instance. The contained object is constructed with
        /// the arguments provided and then moved to a separate thread. The thread is NOT started immediately.
        /// </summary>
        /// <typeparam name="...ArgumentsTypes">The types of the arguments.</typeparam>
        /// <param name="arguments">The arguments themselves, passed with perfect forwarding.</param>
        template<typename... ArgumentsTypes>
        explicit BackgroundService(ArgumentsTypes&&... arguments)
            : service(new ContainedQObjectType(std::forward<ArgumentsTypes>(arguments)...)),
              backgroundThread(new QThread)
        {
            this->service->moveToThread(this->backgroundThread.get());
        }

        // Tries to gracefully stop the thread, waiting up to 5 seconds for it to finish.
        // If it does not finish in that time, it is forcefully terminated.
        ~BackgroundService() noexcept {
            constexpr int maxWaitTime = 5000;
            this->backgroundThread->quit();
            if (!this->backgroundThread->wait(maxWaitTime)) {
                this->backgroundThread->terminate();
                this->backgroundThread->wait();
            }
        }

        // Expected to be used as a part (member) of a QObject, so copying and moving is disallowed.
        BackgroundService(const BackgroundService&) = delete;
        BackgroundService& operator= (const BackgroundService&) = delete;

        BackgroundService(BackgroundService&&) = delete;
        BackgroundService& operator= (BackgroundService&&) = delete;

        /// <summary>
        /// Connects a signal from the contained QObject to a slot in the receiver.
        /// Internally just calls QObject::connect with Qt::QueuedConnection.
        /// </summary>
        /// <typeparam name="SenderMemberFunctionType">The type of function of the contained object.</typeparam>
        /// <typeparam name="ReceiverMemberFunctionType">The type of function of the receiver object.</typeparam>
        /// <param name="signal">The pointer to a function of the contained object.</param>
        /// <param name="receiver">The receiving object.</param>
        /// <param name="slot">The pointer to a function of the receiving object.</param>
        /// <returns>Qt's connection object. May be ignored.</returns>
        template<typename SenderMemberFunctionType, typename ReceiverMemberFunctionType>
        QMetaObject::Connection receive(
            SenderMemberFunctionType signal,
            QObject* receiver,
            ReceiverMemberFunctionType slot
        ) {
            return QObject::connect(this->service.get(), signal, receiver, slot, Qt::QueuedConnection);
        }

        /// <summary>
        /// Executes the provided action in the context of the contained object.
        /// This is done by posting a lambda to the event queue of the thread in which the contained object lives.
        /// Beware that this function is blocking and will wait for the action to complete. You must ensure that
        /// the service is ready to complete the action.
        /// </summary>
        /// <typeparam name="FunctorReturnType">Lambda's result type.</typeparam>
        /// <param name="action">The action itself.</param>
        /// <returns>Lambda's resulting value.</returns>
        template<typename FunctorReturnType>
            requires std::is_default_constructible_v<FunctorReturnType>
        FunctorReturnType send(
            std::function<FunctorReturnType(ContainedQObjectType*)> action
        ) {
            FunctorReturnType result{};
            QMetaObject::invokeMethod(this->service.get(), [this, &action] {
                return action(this->service.get());
            }, Qt::BlockingQueuedConnection, &result);

            return result;
        }

        /// <summary>
        /// Sends a void action to the contained object. Contrary to the templated version, this one does not block
        /// execution and returns immediately.
        /// </summary>
        /// <param name="action">The action to perform.</param>
        void send(
            std::function<void(ContainedQObjectType*)> action
        ) {
            QMetaObject::invokeMethod(this->service.get(), [this, &action] {
                action(this->service.get());
            }, Qt::BlockingQueuedConnection);
        }

        /// <summary>
        /// Starts the service's thread. Should be called only after all connections are set up.
        /// </summary>
        void start() noexcept {
            this->backgroundThread->start();
        }

    private:
        QScopedPointer<ContainedQObjectType> service;
        QScopedPointer<QThread> backgroundThread;
    };
}

#endif