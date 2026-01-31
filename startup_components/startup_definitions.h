#ifndef APPLICATION_STARTUP_COMPONENTS_STARTUP_DEFINITIONS_H
#define APPLICATION_STARTUP_COMPONENTS_STARTUP_DEFINITIONS_H

#define PROJECT_VERSION "2.0.0"

extern const char* STARTUP_COMPONENTS_SHOW_APPLICATION_CONSOLE;
#ifdef STARTUP_COMPONENTS_HIDE_APPLICATION_CONSOLE
const char* STARTUP_COMPONENTS_SHOW_APPLICATION_CONSOLE = "HIDE";
#else
const char* STARTUP_COMPONENTS_SHOW_APPLICATION_CONSOLE = "SHOW";
#endif

#if defined(_MSC_VER) && !defined(__clang__)
// Define the entry point for Unicode-aware applications on Windows.
// It appears that MSVC's own linker can't infer the correct entry point if that
// is defined in a .lib file, so we have to explicitly specify it here.
// Clang's linker seems to handle this situation correctly.
#pragma comment(linker, "/ENTRY:wmainCRTStartup")
#endif

#define APPLICATION_ENTRYPOINT(COMPONENT_NAME, COMPONENT_VERSION, ARGUMENTS_COUNT_NAME, ARGUMENTS_VALUES_NAME) \
    extern const char* STARTUP_COMPONENTS_COMPONENT_NAME; \
    const char* STARTUP_COMPONENTS_COMPONENT_NAME = (COMPONENT_NAME); \
    extern const char* STARTUP_COMPONENTS_BUILD_VERSION; \
    const char* STARTUP_COMPONENTS_BUILD_VERSION = (COMPONENT_VERSION); \
    namespace startup_components { extern int u8main(int, char**); } \
    int startup_components::u8main(int ARGUMENTS_COUNT_NAME, char** ARGUMENTS_VALUES_NAME)

#endif
