package me.hp888.nativeagent.loader;

import java.util.concurrent.ConcurrentHashMap;
import java.net.URLClassLoader;
import java.util.Objects;
import java.util.Map;
import java.net.URL;

/**
 * @author hp888 on 10.11.2019.
 */

public class AgentClassLoader extends URLClassLoader
{
    private final Map<String, byte[]> classBytes;
    private final Map<String, Class<?>> classes;

    public AgentClassLoader(final Map<String, byte[]> classes) {
        super(new URL[0], ClassLoader.getSystemClassLoader());
        this.classes = new ConcurrentHashMap<>();
        this.classBytes = classes;
    }

    @Override
    public Class<?> loadClass(String name) throws ClassNotFoundException {
        Class<?> aClass = classes.get(name);
        if (Objects.nonNull(aClass))
            return aClass;

        final byte[] bytes = classBytes.remove(name);
        if (Objects.nonNull(bytes))
            classes.put(name, aClass = defineClass(name, bytes, 0, bytes.length));

        return Objects.nonNull(aClass)
                ? aClass
                : super.loadClass(name);
    }
}