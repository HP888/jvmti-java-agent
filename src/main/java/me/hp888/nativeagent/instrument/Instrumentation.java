package me.hp888.nativeagent.instrument;

/**
 * @author hp888 on 10.11.2019.
 */

public interface Instrumentation
{
    Class<?>[] getAllLoadedClasses();

    ClassTransformer[] getTransformers();

    void retransformClasses(final Class<?>[] classes);

    Class<?>[] getLoadedClasses(final ClassLoader classLoader);

    void addTransformer(final ClassTransformer classTransformer);
}