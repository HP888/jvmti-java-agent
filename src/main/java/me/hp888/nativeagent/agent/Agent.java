package me.hp888.nativeagent.agent;

import me.hp888.nativeagent.instrument.Instrumentation;

/**
 * @author hp888 on 10.11.2019.
 */

public interface Agent
{
    void onStart(final Instrumentation instrumentation);

    void onShutdown();
}