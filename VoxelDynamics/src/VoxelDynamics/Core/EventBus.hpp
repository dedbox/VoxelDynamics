#pragma once

#include "entt/entt.hpp"

namespace VoxelDynamics::Event
{

class Bus
{
public:
    // singleton instance
    static Bus& Get()
    {
        static Bus instance;
        return instance;
    }

    // Listening -----------------------------------------------------------------------------------

    // prevent move
    Bus(Bus&&)            = delete;
    Bus& operator=(Bus&&) = delete;

    // prevent copy
    Bus(const Bus&)            = delete;
    Bus& operator=(const Bus&) = delete;

    // connect a free function
    template <typename EventType, auto Candidate>
    static void Connect()
    {
        Get()._dispatcher.sink<EventType>().template connect<Candidate>();
    }

    // connect a member function
    template <typename EventType, auto Candidate, typename Type>
    static void Connect(Type* instance)
    {
        Get()._dispatcher.sink<EventType>().template connect<Candidate>(instance);
    }

    // connect a lambda / functor
    template <typename EventType, typename Type>
    static void Connect(Type instance)
    {
        Get()._dispatcher.sink<EventType>().connect(instance);
    }

    // Publishing ----------------------------------------------------------------------------------

    // trigger immediately
    template <typename EventType, typename... Args>
    static void Trigger(Args&&... args)
    {
        Get()._dispatcher.trigger(EventType(std::forward<Args>(args)...));
    }

    // defer processing
    template <typename EventType, typename... Args>
    static void Enqueue(Args&&... args)
    {
        Get()._dispatcher.enqueue(EventType(std::forward<Args>(args)...));
    }

    // process deferred events
    static void Update() { Get()._dispatcher.update(); }

private:
    Bus()  = default;
    ~Bus() = default;

    entt::dispatcher _dispatcher;
};

} // namespace VoxelDynamics::Event
