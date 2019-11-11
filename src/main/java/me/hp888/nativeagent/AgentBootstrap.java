package me.hp888.nativeagent;

import me.hp888.nativeagent.instrument.Instrumentation;
import java.lang.reflect.Field;
import java.util.Objects;

/**
 * @author hp888 on 10.11.2019.
 */

public final class AgentBootstrap
{
    private static AgentMain agent;

    private AgentBootstrap() {}

    public static void startAgent(final Instrumentation instrumentation, final ClassLoader classLoader) throws Throwable {
        final Field systemClassLoaderSetField = ClassLoader.class.getDeclaredField("sclSet");
        systemClassLoaderSetField.setAccessible(true);
        systemClassLoaderSetField.set(null, true);

        final Field systemClassLoaderField = ClassLoader.class.getDeclaredField("scl");
        systemClassLoaderField.setAccessible(true);
        systemClassLoaderField.set(null, classLoader);

        agent = new AgentMain();
        agent.onStart(instrumentation);
    }

    public static void disableAgent() {
        if (Objects.isNull(agent))
            return;

        agent.onShutdown();
    }
}