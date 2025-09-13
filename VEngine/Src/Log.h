#define VENGINE_PRINT(x) \
    {                   \
        std::cout << x; \
    }
#define VENGINE_PRINTLN(x)        \
    {                            \
        VENGINE_PRINT(x << "\n"); \
    }

#define VENGINE_TRUE true
#define VENGINE_FALSE false

#ifdef VENGINE_DEBUG_BUILD
#define VENGINE_DEBUG VENGINE_TRUE
#else
#define VENGINE_RELEASE
#define VENGINE_DEBUG VENGINE_FALSE
#endif

#if VENGINE_DEBUG == VENGINE_TRUE
#define VENGINE_CORE_PRINT(x)           \
    {                                  \
        std::cout << "[ENGINE] " << x; \
    }
#define VENGINE_CORE_PRINTLN(x)                 \
    {                                          \
        std::cout << "[ENGINE] " << x << "\n"; \
    }
#define VENGINE_ERROR(x)                              \
    {                                                \
        std::cerr << "[ENGINE ERROR] " << x << "\n"; \
        exit(EXIT_FAILURE);                          \
    }
#define VENGINE_APP_PRINT(x)          \
    {                                \
        VENGINE_PRINT("[APP] " << x); \
    }
#define VENGINE_APP_PRINTLN(x)          \
    {                                  \
        VENGINE_PRINTLN("[APP] " << x); \
    }
#elif VENGINE_DEBUG == BUILD_FALSE

#define VENGINE_CORE_PRINT(x) \
    {                        \
    }
#define VENGINE_CORE_PRINTLN(x) \
    {                          \
    }
#define VENGINE_APP_PRINT(x) \
    {                       \
    }
#define VENGINE_APP_PRINTLN(x) \
    {                         \
    }
#define VENGINE_ERROR(x) \
    {                   \
    }

#endif