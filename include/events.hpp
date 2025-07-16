// =============================================================================
// Event System - https://github.com/xeroconf/events
// =============================================================================
// Description:
//   A lightweight, header-only library providing a simple event system.
//   Simply include this header file in your project.
//
// License:
//   MIT
//
// Author(s):
//   https://github.com/xeroconf
//
// Version:
//   1.0.0
//
// Date:
//   06/04/2025
//
// =============================================================================

#ifndef AUFORITY_ED_HPP
#define AUFORITY_ED_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <map>
#include <unordered_set>


namespace aufority
{
    namespace es
    {
        namespace detail
        {

            /// <summary>
            /// Compile-time type ID placeholder.
            /// </summary>
            /// <typeparam name="T">The type for which this placeholder struct is instantiated.</typeparam>
            /// <remarks>
            /// This struct is not used for generating actual type IDs.
            /// The value starts at 1 to avoid treating zero as a valid or truthy ID.
            /// </remarks>
            template <typename T>
            struct TypeIdCounter
            {
                static constexpr uint64_t Value = 1;
            };

            /// <summary>
            /// Gets a reference to the static counter used for generating unique type IDs.
            /// </summary>
            /// <returns>
            /// A reference to the counter, initialized to 1. The counter is incremented each time
            /// a new type ID is requested.
            /// </returns>
            inline uint64_t& GetUniqueIdCounter()
            {
                static uint64_t Counter = 1;
                return Counter;
            }

            /// <summary>
            /// Gets a unique, consistent ID for the specified type.
            /// </summary>
            /// <typeparam name="T">The type for which to generate or retrieve a unique ID.</typeparam>
            /// <returns>
            /// A unique ID associated with the type <typeparamref name="T"/>.
            /// The ID remains consistent for the duration of the program.
            /// </returns>
            /// <remarks>
            /// Type IDs start at 1. ID 0 is reserved and considered invalid.
            /// </remarks>
            template <typename T>
            constexpr uint64_t GetTypeId()
            {
                static const uint64_t Id = GetUniqueIdCounter()++;
                return Id;
            }
        }

        /// <summary>
        /// Interface for event processors that process raw event data.
        /// </summary>
        class IEventProcessor
        {
        public:
            virtual ~IEventProcessor() {}

            /// <summary>
            /// Processes an event with the provided data.
            /// </summary>
            /// <param name="InData">A pointer to the raw event data.</param>
            virtual void Process(void* InData) = 0;
        };

        /// <summary>
        /// Concrete event processor that wraps a raw function callback.
        /// </summary>
        class RawEventProcesser : public IEventProcessor
        {
        public:

            /// <summary>
            /// Constructs a RawEventProcessor with a provided handler function.
            /// </summary>
            /// <param name="InFunc">
            /// A function that accepts a <c>void*</c> and processes event data.
            /// </param>
            explicit RawEventProcesser(
                std::function<void(void*)> InFunc
            )
                : Func(std::move(InFunc))
            {
            }

            /// <summary>
            /// Processes/invokes the stored callback function with the provided event data.
            /// </summary>
            /// <param name="InData">A pointer to the raw event data.</param>
            virtual void Process(void* InData) override
            {
                if (Func)
                {
                    Func(InData);
                }
            }

        private:
            /// <summary>
            /// The stored callback function that handles event processing.
            /// </summary>
            std::function<void(void*)> Func;
        };

        /// <summary>
        /// Represents a handle for a subscription to an event, which maintains
        /// information about the event type and the associated event processor.
        /// </summary>
        struct SubscriptionHandle
        {
        public:
            /// <summary>
            /// Friend class that has access to the private members of SubscriptionHandle.
            /// </summary>
            friend class EventDispatcher;

            /// <summary>
            /// Default constructor, initializes the subscription to an unregistered state.
            /// </summary>
            SubscriptionHandle() :
                bRegistered(false),
                EventType(0),
                BoundEventProcessor(nullptr)
            {
            }

        private:

            /// <summary>
            /// Resets the subscription handle to its unregistered state.
            /// </summary>
            inline void Reset()
            {
                bRegistered = false;
                EventType = 0;
                BoundEventProcessor = nullptr;
            }

            /// <summary>
            /// Registers the subscription with an event type and an event processor.
            /// </summary>
            /// <param name="InEventType">The type of event to subscribe to.</param>
            /// <param name="InEventProcessor">The processor that processes the event.</param>
            /// <returns>
            /// <c>true</c> if registration was successful; <c>false</c> if invalid parameter(s) or the handler is already registered.
            /// </returns>
            bool Register(
                const uint64_t InEventType,
                const std::shared_ptr<IEventProcessor>& InEventProcessor
            )
            {
                if (!InEventType)
                {
                    return false;
                }

                if (!InEventProcessor)
                {
                    return false;
                }

                if (bRegistered)
                {
                    return false;
                }

                bRegistered = true;
                EventType = InEventType;
                BoundEventProcessor = InEventProcessor;

                return true;
            }

            /// <summary>
            /// Checks whether this subscription is currently registered.
            /// </summary>
            /// <returns>
            /// <c>true</c> if the subscription is registered; <c>false</c> otherwise.
            /// </returns>
            inline bool IsRegistered() const { return bRegistered; }

            /// <summary>
            /// Retrieves the processor bound to the subscription.
            /// </summary>
            /// <returns>The processor associated with the subscription.</returns>
            inline std::shared_ptr<IEventProcessor>& GetBoundProcessor() { return BoundEventProcessor; }

            /// <summary>
            /// Retrieves the event type ID associated with the subscription.
            /// </summary>
            /// <returns>The event type ID.</returns>
            inline uint64_t GetEventTypeId() const { return EventType; }

        private:
            /// <summary>
            /// Indicates whether the subscription is registered.
            /// </summary>
            bool bRegistered;

            /// <summary>
            /// The event type ID this handle is associated with.
            /// </summary>
            uint64_t EventType;

            /// <summary>
            /// The processor responsible for processing the event when triggered.
            /// </summary>
            std::shared_ptr<IEventProcessor> BoundEventProcessor;
        };

        /// <summary>
        /// Dispatcher responsible for subscribing, unsubscribing, and emitting events to the appropriate handlers.
        /// </summary>
        class EventDispatcher
        {
        public:
            /// <summary>
            /// Default constructor for the <see cref="EventDispatcher"/>.
            /// </summary>
            EventDispatcher() {}

            /// <summary>
            /// Subscribes a handler function to a specific event type, using the <see cref="RawEventProcessor"/>.
            /// </summary>
            /// <typeparam name="EventType">The type of the event to subscribe to.</typeparam>
            /// <typeparam name="HandlerFunction">The type of the handler function.</typeparam>
            /// <param name="InEventHandle">A reference to the <see cref="SubscriptionHandle"/> that will represent the subscription.</param>
            /// <param name="InHandlerFunc">The function to handle the event.</param>
            /// <returns><c>true</c> if the subscription was successful; <c>false</c> otherwise.</returns>
            template<typename EventType, typename HandlerFunctionType>
            inline bool Subscribe(
                SubscriptionHandle& InEventHandle,
                HandlerFunctionType InHandlerFunc
            )
            {
                // Get the unique event type ID for the given EventType
                uint64_t TypeId = detail::GetTypeId<EventType>();

                // Delegate to the internal subscribe method to register the handler
                return SubscribeInternal(
                    TypeId,
                    InEventHandle,
                    std::make_shared<RawEventProcessor>
                    (
                        [InHandlerFunc = std::move(InHandlerFunc)](void* InEventData)
                        {
                            EventType& Ev = *static_cast<EventType*>(InEventData);
                            InHandlerFunc(Ev);
                        }
                    )
                );
            }

            /// <summary>
            /// Subscribes an event handler processor to a specific event type.
            /// </summary>
            /// <typeparam name="EventType">The type of the event to subscribe to.</typeparam>
            /// <param name="InEventHandle">A reference to the <see cref="SubscriptionHandle"/> that will represent the subscription.</param>
            /// <param name="InEventProcessor">A shared pointer to the processor that will handle the event.</param>
            /// <returns>
            /// <returns><c>true</c> if the subscription was successful; <c>false</c> otherwise.</returns>
            /// </returns>
            template<typename EventType>
            inline bool Subscribe(
                SubscriptionHandle& InEventHandle,
                std::shared_ptr<IEventProcessor> InEventProcessor
            )
            {
                // Get the unique event type ID for the given EventType
                uint64_t TypeId = detail::GetTypeId<EventType>();

                // Delegate to the internal subscribe method to register the handler
                return SubscribeInternal(
                    TypeId,
                    InEventHandle,
                    InEventProcessor
                );
            }

            /// <summary>
            /// Unsubscribes from an event and removes the associated handler.
            /// </summary>
            /// <param name="InEventHandle">A reference to the <see cref="SubscriptionHandle"/> to unsubscribe.</param>
            /// <returns><c>true</c> if the unsubscription was successful; <c>false</c> otherwise.</returns>
            inline bool Unsubscribe(SubscriptionHandle& InEventHandle)
            {
                uint64_t TypeId = InEventHandle.GetEventTypeId();
                return UnsubscribeInternal(TypeId, InEventHandle);
            }

            /// <summary>
            /// Emits an event of a specific type with the provided data.
            /// </summary>
            /// <typeparam name="EventType">The type of the event to emit.</typeparam>
            /// <param name="InData">The data to be passed to the event handler.</param>
            /// <returns><c>true</c> if the event was successfully emitted; <c>false</c> otherwise.</returns>
            template<typename EventType>
            inline bool Emit(EventType& InData)
            {
                uint64_t TypeId = detail::GetTypeId<EventType>();
                return EmitInternal(TypeId, &InData);
            }

            /// <summary>
            /// Copy assignment operator
            /// </summary>
            /// <param name="Other">The other <see cref="EventDispatcher"/> to assign from.</param>
            void operator=(const EventDispatcher& Other)
            {
                *this = Other;
            }

        private:

            /// <summary>
            /// Internal method to register a subscription.
            /// </summary>
            /// <param name="InEventType">The event type ID to subscribe to.</param>
            /// <param name="InEventHandle">The subscription handle to register.</param>
            /// <param name="InEventProcessor">The event processor to associate with the subscription.</param>
            /// <returns><c>true</c> if the subscription was successfully registered; <c>false</c> otherwise.</returns>
            bool SubscribeInternal(
                uint64_t InEventType,
                SubscriptionHandle& InEventHandle,
                std::shared_ptr<IEventProcessor> InEventProcessor
            )
            {
                std::lock_guard<std::mutex> LockGuard(EmissionMutex);

                if (InEventHandle.IsRegistered())
                {
                    return false;
                }

                if (!InEventHandle.Register(InEventType, InEventProcessor))
                {
                    return false;
                }

                TypeToHandlerMap[InEventType].insert(&InEventHandle);

                return true;
            }

            /// <summary>
            /// Internal method to unsubscribe from an event.
            /// </summary>
            /// <param name="InEventType">The event type ID to unsubscribe from.</param>
            /// <param name="InEventHandle">The subscription handle to remove.</param>
            /// <returns><c>true</c> if the unsubscription was successful; <c>false</c> otherwise.</returns>
            bool UnsubscribeInternal(uint64_t InEventTypeId, SubscriptionHandle& InEventHandle)
            {
                std::lock_guard<std::mutex> LockGuard(EmissionMutex);

                if (!InEventHandle.IsRegistered())
                {
                    return false;
                }

                // Erase handler from the map
                TypeToHandlerMap[InEventTypeId].erase(&InEventHandle);

                InEventHandle.Reset();

                return true;
            }

            /// <summary>
            /// Internal method to emit an event to its subscribers.
            /// </summary>
            /// <param name="InEventType">The event type ID to emit.</param>
            /// <param name="InData">The data to pass to the event handlers.</param>
            /// <returns><c>true</c> if the event was successfully emitted; <c>false</c> otherwise.</returns>
            bool EmitInternal(uint64_t InEventTypeId, void* InData)
            {
                std::lock_guard<std::mutex> LockGuard(EmissionMutex);

                auto HandlesIter = TypeToHandlerMap.find(InEventTypeId);

                if (HandlesIter == TypeToHandlerMap.end())
                {
                    return false;
                }

                auto& Handlers = HandlesIter->second;

                if (Handlers.empty())
                {
                    return false;
                }

                for (auto Handlers : Handlers)
                {
                    Handlers->GetBoundProcessor()->Process(InData);
                }

                // An event was processed, so we return true.
                return true;
            }

        private:

            /// <summary>
            /// A map that stores event type IDs and the corresponding set of subscription handles.
            /// </summary>
            std::map<uint64_t, std::unordered_set<SubscriptionHandle*>> TypeToHandlerMap;

            /// <summary>
            /// Mutex to synchronize event dispatching, ensuring thread safety.
            /// </summary>
            mutable std::mutex EmissionMutex = {};
        };
    }
}

#endif
