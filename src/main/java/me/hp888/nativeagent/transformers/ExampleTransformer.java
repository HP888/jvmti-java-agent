package me.hp888.nativeagent.transformers;

import me.hp888.nativeagent.instrument.ClassTransformer;
import java.security.ProtectionDomain;
import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;
import javassist.*;

/**
 * @author hp888 on 10.11.2019.
 */

public class ExampleTransformer implements ClassTransformer
{
    @Override
    public byte[] transform(ClassLoader loader, String className, Class<?> classBeingRedefined, ProtectionDomain protectionDomain, byte[] classfileBuffer) {
        if (!classBeingRedefined.getName().equals("java.lang.Integer"))
            return classfileBuffer;

        try {
            final ClassPool classPool = ClassPool.getDefault();
            final CtClass ctClass = classPool.getCtClass(classBeingRedefined.getName());
            if (ctClass.isFrozen())
                ctClass.detach();

            final CtMethod ctMethod = ctClass.getDeclaredMethod("valueOf");
            ctMethod.insertBefore("System.err.println(\"[Agent] Hello from CustomNativeAgent!\");");

            classfileBuffer = ctClass.toBytecode();
            ctClass.detach();

            Logger.getLogger(getClass().getSimpleName()).info("Successfully retransformed " + classBeingRedefined.getName() + "!");
        } catch (final CannotCompileException | IOException | NotFoundException ex) {
            Logger.getLogger(getClass().getSimpleName()).log(Level.SEVERE, "Exception thrown when transforming class!", ex);
        }

        return classfileBuffer;
    }
}