#include <functional>
#include <iostream>
#include <mutex>
#include <queue>

#include "module.h"

struct Box {

};

struct Event {
    std::string type;
    Box payload;
};

class EventListener {
    virtual void onEvent(Event event) = 0;
};

class EventBus {
private:
    std::mutex m_subscriptionMutex;
    std::unordered_map<std::string, std::vector<EventListener*>> m_listeners;
    std::unordered_map<std::string, std::vector<std::function<void(Event)>>> m_callbacks;

    std::mutex m_eventMutex;
    std::queue<Event> m_eventQueue;

public:
    void publish(Event && event);

    void subscribe(std::string type, std::function<void(Event)> callback);
    void subscribe(std::string type, EventListener* listener);

    void unsubscribe(EventListener* listener);
    void unsubscribe(std::string type, EventListener* listener);
};

void EventBus::publish(Event && event)
{
    std::lock_guard<std::mutex> lock(m_eventMutex);
    m_eventQueue.push(std::move(event));
}




int main()
{
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
