
# Events
A simple header only, compile time event system with modern C++.

## Usage/Example

```cpp
using namespace aufority::es;

struct GreetingEvent 
{
    std::string Name;
}

int main() {
    // Create a dispatcher
    EventDispatcher dispatcher;

    // Create an instance of the handle
    SubscriptionHandle greetingHandle;

    // Subscribe to the event
    dispatcher.Subscribe<GreetingEvent>(
        greetingHandle, 
        [](const GreetingEvent& ev) 
        {
            printf("Say hello to %s!", ev.Name);
        }
    );

    // Construct the event
    GreetingEvent ev;
    ev.Name = "Joe";

    // Emit the event to any subscribed handlers
    dispatcher.Emit(ev);

    // Unsubscribe from the event
    dispatcher.Unsubscribe(greetingHandle);
}
```
