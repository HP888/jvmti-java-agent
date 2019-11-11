package me.hp888.nativeagent.instrument;

import java.security.ProtectionDomain;

/**
 * @author hp888 on 10.11.2019.
 */

public interface ClassTransformer
{
    byte[] transform(final ClassLoader loader, final String className, final Class<?> classBeingRedefined, final ProtectionDomain protectionDomain, final byte[] classfileBuffer);
}