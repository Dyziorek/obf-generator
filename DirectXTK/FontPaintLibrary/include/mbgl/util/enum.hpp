#ifndef MBGL_UTIL_ENUM
#define MBGL_UTIL_ENUM

#include <iosfwd>
#include <string>

namespace mbgl {

template <typename Type>
struct EnumValue {
    Type value;
    char *name;
	EnumValue(){}
	EnumValue(Type INvalue, char* inText)
	{
		value = INvalue;
		name = inText;
	}
};

template <typename EnumName, const EnumValue<EnumName> *names, const size_t length>
struct Enum {
    using Type = EnumName;
    Type value;
    static const size_t l = length;
private:
    static inline bool compare(const char *a, const char *b) {
        return *a == *b && (*a == '\0' || compare(a + 1, b + 1));
    }
    static inline const char *lookup_type(Type e, EnumValue<Type> const * const list, size_t r) {
        return r == 0 ? "" : list->value == e ? list->name : lookup_type(e, list + 1, r - 1);
    }
    static inline Type lookup_name(const char *n, EnumValue<Type> const * const list, size_t r) {
        return r == 0 ? Type(-1) : compare(list->name, n) ? list->value : lookup_name(n, list + 1, r - 1);
    }
public:
    inline Enum(const char *n) : value(lookup_name(n, names, length)) {}
    inline Enum(const std::string &n) : value(lookup_name(n.c_str(), names, length)) {}
    inline Enum(Type t) : value(t) {}

    inline void operator=(const char *n) { value = lookup_name(n, names, length); }
    inline void operator=(const std::string &n) { *this = n.c_str(); }
    inline void operator=(Type t) { value = t; }

    inline bool valid() const { return value != Type(-1); }

    inline const char *c_str() const { return lookup_type(value, names, length); }
    inline std::string str() const { return c_str(); }

    inline operator Type() const { return value; }
};

#ifndef _MSC_VER
#define MBGL_DEFINE_ENUM_CLASS(name, type, strings...) \
    const ::mbgl::EnumValue<type> type##_names[] = strings; \
    using name = ::mbgl::Enum<type, type##_names, sizeof(type##_names) / sizeof(::mbgl::EnumValue<type>)>; \
    inline std::ostream& operator<<(std::ostream& os, type t) { return os << name(t).str(); }
#else
#define MBGL_DEFINE_ENUM_CLASS_NAME(name, type, num, ...) \
    ::mbgl::EnumValue<type> type##_names[num] = {__VA_ARGS__}; \
    using name = ::mbgl::Enum<type, type##_names, sizeof(type##_names) / sizeof(::mbgl::EnumValue<type>)>; \
    inline std::ostream& operator<<(std::ostream& os, type t) { return os << name(t).str(); }

#define MBGL_DEFINE_ENUM_CLASS_INIT(name, type, num, arg1, arg2) \
    type##_names[num] = ::mbgl::EnumValue<type>(arg1, arg2); 
#endif
}

#endif

