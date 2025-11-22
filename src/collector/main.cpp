#include <initializer_list>
#include <signal.h>
#include <unistd.h>

#include "Collector.h"

// https://gist.github.com/azadkuh/a2ac6869661ebd3f8588
void catchUnixSignals(std::initializer_list<int> quitSignals)
{
    auto handler = [](int /*sig*/) -> void
    {
        // blocking and not aysnc-signal-safe func are valid
        // printf("\nquit the application by signal(%d).\n", sig);
        QCoreApplication::quit();
    };

    sigset_t blocking_mask;
    sigemptyset(&blocking_mask);
    for (auto sig : quitSignals)
        sigaddset(&blocking_mask, sig);

    struct sigaction sa;
    sa.sa_handler = handler;
    sa.sa_mask    = blocking_mask;
    sa.sa_flags   = 0;

    for (auto sig : quitSignals)
        sigaction(sig, &sa, nullptr);
}

int main(int argc, char *argv[])
{
    Collector collector(argc, argv);

    catchUnixSignals({SIGQUIT, SIGINT, SIGTERM, SIGHUP});

    return collector.exec();
}
