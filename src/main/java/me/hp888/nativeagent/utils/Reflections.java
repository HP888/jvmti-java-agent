package me.hp888.nativeagent.utils;

import java.lang.reflect.Method;
import java.util.Arrays;

/**
 * @author hp888 on 10.11.2019.
 */

public final class Reflections
{
    private Reflections() {}

    public static Method getMethod(final Class<?> aClass, final String name) {
        try {
            for (final Method method : aClass.getMethods()) {
                if (method.getName().equals(name)) {
                    return method;
                }
            }

            return null;
        } catch (final Throwable throwable) {
            JavaUtil.printStacktrace(throwable);
        }

        return null;
    }

    public static Object invokeMethod(final Method method, final Object instance, final Object... args) {
        try {
            if (method == null) {
                System.err.println("Method == null, args: " + Arrays.toString(args));
                return null;
            }

            return method.invoke(instance, args);
        } catch (final Throwable throwable) {
            JavaUtil.printStacktrace(throwable);
        }

        return null;
    }
}