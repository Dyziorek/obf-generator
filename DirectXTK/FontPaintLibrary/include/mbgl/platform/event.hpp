#ifndef MBGL_PLATFORM_EVENT
#define MBGL_PLATFORM_EVENT

#include <mbgl/util/enum.hpp>

#include <cstdint>

namespace mbgl {

enum class EventSeverity : uint8_t {
    Debug,
    Info,
    Warning,
    Error,
};

//::mbgl::EnumValue<EventSeverity> EventSeverityClass_names[5];
//using EventSeverityClass = ::mbgl::Enum < EventSeverity, EventSeverityClass_names, 5>;
//inline std::ostream& operator<<(std::ostream& os, EventSeverity t) { return os << EventSeverityClass(t).str(); }

#ifndef _MSC_VER
MBGL_DEFINE_ENUM_CLASS(EventSeverityClass, EventSeverity, {
    { EventSeverity::Debug, "DEBUG" },
    { EventSeverity::Info, "INFO" },
    { EventSeverity::Warning, "WARNING" },
    { EventSeverity::Error, "ERROR" },
    { EventSeverity(-1), "UNKNOWN" },
});
#else
MBGL_DEFINE_ENUM_CLASS_NAME(EventSeverityClass, EventSeverity, 5);
MBGL_DEFINE_ENUM_CLASS_INIT(EventSeverityClass, EventSeverity, 0, EventSeverity::Debug, "DEBUG");
MBGL_DEFINE_ENUM_CLASS_INIT(EventSeverityClass, EventSeverity, 1, EventSeverity::Info, "INFO");
MBGL_DEFINE_ENUM_CLASS_INIT(EventSeverityClass, EventSeverity, 2, EventSeverity::Warning, "WARNING");
MBGL_DEFINE_ENUM_CLASS_INIT(EventSeverityClass, EventSeverity, 3, EventSeverity::Error, "ERROR");
MBGL_DEFINE_ENUM_CLASS_INIT(EventSeverityClass, EventSeverity, 4, EventSeverity(-1), "UNKNOWN");
#endif

enum class Event : uint8_t {
    General,
    Setup,
    Shader,
    ParseStyle,
    ParseTile,
    Render,
    Database,
    HttpRequest,
    Sprite,
    OpenGL,
};

MBGL_DEFINE_ENUM_CLASS(EventClass, Event, {
    { Event::General, "General" },
    { Event::Setup, "Setup" },
    { Event::Shader, "Shader" },
    { Event::ParseStyle, "ParseStyle" },
    { Event::ParseTile, "ParseTile" },
    { Event::Render, "Render" },
    { Event::Database, "Database" },
    { Event::HttpRequest, "HttpRequest" },
    { Event::Sprite, "Sprite" },
    { Event::OpenGL, "OpenGL" },
    { Event(-1), "Unknown" },
});


struct EventPermutation {
    const EventSeverity severity;
    const Event event;

    constexpr bool operator==(const EventPermutation &rhs) const {
        return severity == rhs.severity && event == rhs.event;
    }
};

constexpr EventSeverity disabledEventSeverities[] = {
#if DEBUG
    EventSeverity(-1) // Avoid zero size array
#else
    EventSeverity::Debug
#endif
};

constexpr Event disabledEvents[] = {
    Event(-1) // Avoid zero size array
};

constexpr EventPermutation disabledEventPermutations[] = {
    { EventSeverity::Debug, Event::Shader }
};

}

#endif
