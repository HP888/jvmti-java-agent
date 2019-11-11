package me.hp888.nativeagent;

import me.hp888.nativeagent.agent.Agent;
import me.hp888.nativeagent.instrument.Instrumentation;
import me.hp888.nativeagent.transformers.ExampleTransformer;
import java.util.Arrays;

/**
 * @author hp888 on 10.11.2019.
 */

public class AgentMain implements Agent
{
    @Override
    public void onStart(Instrumentation instrumentation) {
        System.out.println("[AgentMain] Started agent!");
        instrumentation.addTransformer(new ExampleTransformer());
        instrumentation.retransformClasses(Arrays.stream(instrumentation.getAllLoadedClasses())
                .filter(aClass -> aClass.getName().startsWith("java.lang.Integer"))
                .toArray(Class[]::new));
    }

    @Override
    public void onShutdown() {
        System.out.println("[AgentMain] Disabled agent!");
    }
}