package me.hp888.nativeagent.instrument.impl;

import me.hp888.nativeagent.NativeAccesses;
import me.hp888.nativeagent.data.Settings;
import me.hp888.nativeagent.instrument.ClassTransformer;
import me.hp888.nativeagent.instrument.Instrumentation;
import java.io.File;

/**
 * @author hp888 on 10.11.2019.
 */

public class InstrumentationImpl implements Instrumentation
{
    static {
        final File file = new File(Settings.native_path);
        if (!file.exists()) {
            System.err.println("Dynamic link library not found in " + Settings.native_path + "!");
            throw new IllegalArgumentException("Dynamic link library not found in " + Settings.native_path + "!");
        }

        System.load(file.getAbsolutePath());
    }

    @Override
    public ClassTransformer[] getTransformers() {
        return NativeAccesses.getTransformersAsArray();
    }

    @Override
    public native Class<?>[] getAllLoadedClasses();

    @Override
    public native void retransformClasses(Class<?>[] classes);

    @Override
    public native Class<?>[] getLoadedClasses(ClassLoader classLoader);

    @Override
    public void addTransformer(final ClassTransformer classTransformer) {
        NativeAccesses.addTransformer(classTransformer);
    }
}