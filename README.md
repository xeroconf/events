
# Events

A simple header only, compile time, event dispatcher.




## Usage/Examples

```cpp
using namespace aufority;

struct SomethingHappenedEvent 
{
    std::string Data;
}

int main() {

    // Create an instance of the dispatcher
    Dispatcher Dispatcher;

    // Create an instance of the handle
    EventHandle Handle;

    // Register the event with the system
    Dispatcher.Subscribe<SomethingHappenedEvent>(
        Handle, 
        [](const SomethingHappenedEvent& Event) 
        {
            printf("What happened: %s", Event.Data);
        }
    );

    // Create the event
    SomethingHappenedEvent Event;
    Event.Data = "Something happend!";

    // Fire the event
    bool WasHandled = Dispatcher.Emit(Event); // Returns true if a handler was notified of this event. In this example, it would be true.

    // Unsubscribe from the event, the handler will receive no more.
    Dispatcher.Unsubscribe(Handle);
}

```


### Custom Listeners
In some cases, the basic raw listener may not be sufficient for your needs. With custom listeners, you can define your own listener type to handle events in a way that best suits your application.
```cpp
// We may want to forward events to a remote service via HTTP.
class EventToRemoteListener : public IEventListener 
{
public:

    EventToRemoteListener(
        std::string& InEndpoint
    ) :
        Endpoint(Endpoint)
    {};

private:

	virtual void Trigger(void* InData) override
    {
        // send event via http to remote service...?
    }

    std::string Endpoint;
}
```

or extend the raw listener...
```cpp
class LimitedRawEventListener : public RawEventListener 
{
public:

    LimitedRawEventListener(
        int InLimit,
        std::function<void(void*)> InFunc
    ) :
        RawEventListener(InFunc),
        Limit(InLimit)
    {};

private:

	virtual void Trigger(void* Data) override
    {
        if (Current >= Limit)
            return; // We reached the handling limit, don't invoke anymore.

        RawEventListener::Trigger(Data);

        Current++;
    }

    int Limit = 0;
    int Current = 0;
}
```

We can then register it...
```cpp
auto RemoteListener = new EventToRemoteListener("https://example.com/events");
Dispatcher.Subscribe(HandleA, RemoteListener);
Dispatcher.Subscribe(HandleB, RemoteListener);

Dispatcher.Subscribe(
    HandleC, 
    new LimitedRawEventListener(
        12, // Handler will only execute the function below 12 times
        [](const void* Data)
        {
            print("Hello World");
        }
    )
);
```


### Event Types
Since event identifiers are determined at compile time, any type (except generics) can be used as an event. This eliminates the need for events to inherit from a base class, providing greater flexibility.
```cpp
Dispatcher.Subscribe<std::string>(
    Handle, 
    [](const std::string& Content) 
    {
        printf("Received string: %s", Content);
    }
);

Dispatcher.Emit(std::string("Hello World"));
```

### Handle
To subscribe to an event, you need a handle instance. The handle ensures that you don't subscribe to the same event multiple times while also allowing you to unsubscribe when needed.

You must keep the handle instance alive for as long as you need the handler to remain active.
```cpp
Dispatcher.Subscribe<Event>(
    Handle, 
    [](const Event& Content) {}
);

if (Handle.IsRegistered())
{
    printf("This handle is registered to event ID %i", Handle.GetType());
}

```
