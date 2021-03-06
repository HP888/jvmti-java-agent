# jvmti-java-agent

Agent written in c++ for Java
Like basic Java Agent but more lightweight
Currently works only on Windows.

Default locations: (you can change NativeAgent.dll location in me/hp888/nativeagent/data/Settings)
- C:/agent.jar
- C:/NativeAgent.dll

(I'm not coding in C++ every day so please don't hate me for bad code) 

# Informations about project

Project should work on Java 8 and later versions.

# How to inject agent?

Download & run ProcessHacker -> PPM on process -> Miscellaneous -> Inject DLL -> Select DLL from C:/NativeAgent.dll

![alt text](https://cdn.discordapp.com/attachments/628611450331463690/643546428450406421/processhacker-inject-dll.png)

# Examples

### Getting all loaded Classes

```
public class AgentMain implements Agent
{
    @Override
    public void onStart(Instrumentation instrumentation) {
        System.out.println("[AgentMain] Started agent!");
        Arrays.stream(instrumentation.getAllLoadedClasses())
                .forEach(aClass -> System.out.println("Class name: " + aClass.getName()));
    }

    @Override
    public void onShutdown() {
        System.out.println("[AgentMain] Disabled agent!");
    }
}
```

### Retransforming Classes

```
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
```

### Class Transformer
```
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
```

# Things to do

- fix retransforming many classes (now causes application crash)
- fix onShutdown method (now this method never will be called)
- add more options to instrumentation like removeTransformer
