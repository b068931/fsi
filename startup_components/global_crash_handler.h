#ifndef STARTUP_COMPONENTS_GLOBAL_CRASH_HANDLER_H
#define STARTUP_COMPONENTS_GLOBAL_CRASH_HANDLER_H

namespace startup_components::crash_handling {
    bool install_global_crash_handler();
    void remove_global_crash_handler();
}

#endif
