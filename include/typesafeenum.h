// https://github.com/0xfeedc0de64/cpputils

#define CPP_STRINGIFY2(x) #x
#define CPP_STRINGIFY(x) CPP_STRINGIFY2(x)

#define DECLARE_TYPESAFE_ENUM_HELPER1(name, ...) name __VA_ARGS__ ,
#define DECLARE_TYPESAFE_ENUM_HELPER2(name, ...) case TheEnum::name: return #name;
#define DECLARE_TYPESAFE_ENUM_HELPER3(name, ...) cb(TheEnum::name, CPP_STRINGIFY(name));

#define DECLARE_TYPESAFE_ENUM(Name, Derivation, Values) \
    enum class Name Derivation \
    { \
        Values(DECLARE_TYPESAFE_ENUM_HELPER1) \
    }; \
    inline std::string toString(Name value) \
    { \
        switch (value) \
        { \
        using TheEnum = Name; \
        Values(DECLARE_TYPESAFE_ENUM_HELPER2) \
        } \
        return "Unknown"; \
    } \
    template<typename T> \
    void iterate##Name(T &&cb) \
    { \
        using TheEnum = Name; \
        Values(DECLARE_TYPESAFE_ENUM_HELPER3) \
    }