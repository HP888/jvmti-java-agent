#include "me_hp888_nativeagent_instrument_impl_InstrumentationImpl.h"
#include "NativeAgent.h"
#include "main.h"
#include <fstream>
#include <iostream>
#include "pch.h"

using namespace std;

jrawMonitorID vmtrace_lock;
jlong start_time;

jvmtiEnv* jvmti;
JavaVM* jvm;
JNIEnv* env;

NativeAgent::NativeAgent() {
	jsize count;
	if (JNI_GetCreatedJavaVMs((JavaVM * *)& jvm, 1, &count) != JNI_OK || count == 0) {
		cout << "Failed to get the JVM" << endl;
		return;
	}

	jint res = jvm->GetEnv((void**)& env, JNI_VERSION_1_6);
	if (res == JNI_EDETACHED) {
		res = jvm->AttachCurrentThread((void**)& env, nullptr);
	}

	if (res != JNI_OK) {
		cout << "Failed to attach to the thread" << endl;
		return;
	}

	res = jvm->GetEnv((void**)& jvmti, JVMTI_VERSION_1_0);
	if (res != JNI_OK) {
		cerr << "Failed to get JVMTI env!" << endl;
		return;
	}

	cout << "[Agent] Attached to JVM" << endl;
	cout << "[Agent] Developed by HP888" << endl;
}

static bool loaded;
static NativeAgent* instance;

/* utilities */

static jmethodID getMethod = NULL, invokeMethod = NULL;
static jclass nativeAccesses = NULL, reflections = NULL;
static string* names = NULL;

jbyteArray asByteArray(JNIEnv* env, const unsigned char* buf, int len) {
	jbyteArray array = env->NewByteArray(len);
	env->SetByteArrayRegion(array, 0, len, (const jbyte*)buf);
	return array;
}

unsigned char* asUnsignedCharArray(JNIEnv* env, jbyteArray array) {
	int len = env->GetArrayLength(array);
	unsigned char* buf = new unsigned char[len];
	env->GetByteArrayRegion(array, 0, len, reinterpret_cast<jbyte*>(buf));
	return buf;
}

static int index, classesToRedefine;

static int redefine(jvmtiEnv* jvmti, jvmtiClassDefinition* class_def) {
	if (!jvmti->RedefineClasses(classesToRedefine, class_def))
		return 0;

	return 1;
}

/*
	// saving classes

	ofstream f;
	string s("C:/Users/hp888/OneDrive/Pulpit/test.class");
	f.open(s, std::ios::out | std::ios::binary);
	f.write((const char*) newChars, newLength);
	f.flush();
	f.close();
*/

void JNICALL classTransformerHook(jvmtiEnv* jvmti, JNIEnv* env, jclass class_being_redefined, jobject loader, const char* name, jobject protection_domain, jint data_len, const unsigned char* data, jint* new_data_len, unsigned char** new_data) {
	if (name == NULL)
		return;

	if (!loaded) {
		jvmti->Allocate(data_len, new_data);

		*new_data_len = data_len;
		memcpy(*new_data, data, data_len);
	} else {
		const jclass stringCls = env->FindClass("java/lang/String");
		const jmethodID startsWith = env->GetMethodID(stringCls, "startsWith", "(Ljava/lang/String;)Z");
		const jstring javaClassName = env->NewStringUTF(name),
			javaIndexName = env->NewStringUTF(names[index].c_str());

		if (!env->CallBooleanMethod(javaIndexName, startsWith, javaClassName)) {
			jvmti->Allocate(data_len, new_data);
			*new_data_len = data_len;
			memcpy(*new_data, data, data_len);
			return;
		}

		jvmti->Allocate(data_len, new_data);
		*new_data_len = data_len;
		memcpy(*new_data, data, data_len);

		const jobject method = env->CallStaticObjectMethod(reflections, getMethod, nativeAccesses, env->NewStringUTF("transformClass"));
		const jclass arrayCls = env->FindClass("java/lang/reflect/Array");
		const jclass object = env->FindClass("java/lang/Object");
		const jmethodID newInstance = env->GetStaticMethodID(arrayCls, "newInstance", "(Ljava/lang/Class;I)Ljava/lang/Object;");
		const jobjectArray newArray = (jobjectArray)env->CallStaticObjectMethod(arrayCls, newInstance, object, (jint)5);
		const jbyteArray plainBytes = asByteArray(env, *new_data, *new_data_len);
		const jbyteArray byteArray = env->NewByteArray(0);
		env->SetObjectArrayElement(newArray, 0, loader);
		env->SetObjectArrayElement(newArray, 1, env->NewStringUTF(name));
		env->SetObjectArrayElement(newArray, 2, class_being_redefined);
		env->SetObjectArrayElement(newArray, 3, protection_domain);
		env->SetObjectArrayElement(newArray, 4, plainBytes);

		const jbyteArray newByteArray = (jbyteArray)env->CallStaticObjectMethod(reflections, invokeMethod, method, NULL, newArray);
		unsigned char* newChars = asUnsignedCharArray(env, newByteArray);
		const jint newLength = (jint)env->GetArrayLength(newByteArray);

		jvmti->Allocate(newLength, new_data);
		*new_data_len = newLength;
		memcpy(*new_data, newChars, newLength);

		index++;
		jvmtiClassDefinition def = jvmtiClassDefinition();
		redefine(jvmti, &def);
		cout << "[Agent] Transformed class: " << name << endl;
	}
}

void initializeJvmti(NativeAgent* agent) {
	cout << "[Agent] Setting up JVMTI!" << endl;

	agent->jvmti->CreateRawMonitor("vmtrace_lock", &vmtrace_lock);
	agent->jvmti->GetTime(&start_time);

	if (agent->env->ExceptionOccurred()) {
		agent->env->ExceptionDescribe();
		return;
	}

	jvmtiCapabilities capabilities = { 0 };
	capabilities.can_generate_all_class_hook_events = 1;
	capabilities.can_retransform_any_class = 1;
	capabilities.can_retransform_classes = 1;
	capabilities.can_redefine_any_class = 1;
	capabilities.can_redefine_classes = 1;
	agent->jvmti->AddCapabilities(&capabilities);

	if (agent->env->ExceptionOccurred()) {
		agent->env->ExceptionDescribe();
		return;
	}

	cout << "[Agent] JVMTI was set!" << endl;
}

jbyteArray toByteArray(JNIEnv* env, jobject inputStream) {
	const jclass byteArrayOutputStreamClass = env->FindClass("java/io/ByteArrayOutputStream");
	const jclass inputStreamClass = env->FindClass("java/io/InputStream");

	const jmethodID byteArrayOutputStreamConstructor = env->GetMethodID(byteArrayOutputStreamClass, "<init>", "()V");
	const jmethodID readMethod = env->GetMethodID(inputStreamClass, "read", "([B)I");

	const jmethodID writeMethod = env->GetMethodID(byteArrayOutputStreamClass, "write", "([BII)V");
	const jmethodID toByteArray = env->GetMethodID(byteArrayOutputStreamClass, "toByteArray", "()[B");

	const jobject byteArrayOutputStream = env->NewObject(byteArrayOutputStreamClass, byteArrayOutputStreamConstructor);
	const jbyteArray buffer = env->NewByteArray(4096);

	jlong count;
	jint read;
	for (count = 0; (read = env->CallIntMethod(inputStream, readMethod, buffer)) != -1; count += read) {
		env->CallVoidMethod(byteArrayOutputStream, writeMethod, buffer, (jint)0, read);
	}

	const jbyteArray bytes = (jbyteArray)env->CallObjectMethod(byteArrayOutputStream, toByteArray);
	return (jbyteArray)env->CallObjectMethod(byteArrayOutputStream, toByteArray);
}

jobject addClassesToMap(JNIEnv* env, jstring agentPath) {
	const jclass stringCls = env->FindClass("java/lang/String");
	const jstring dotString = env->NewStringUTF("."),
		classString = env->NewStringUTF(".class"),
		slashString = env->NewStringUTF("/"),
		spaceString = env->NewStringUTF(" "),
		emptyString = env->NewStringUTF("");

	const jclass fileClass = env->FindClass("java/io/File");
	const jclass fileInputStream = env->FindClass("java/io/FileInputStream");
	const jmethodID fileConstructor = env->GetMethodID(fileClass, "<init>", "(Ljava/lang/String;)V");
	const jobject fileInstance = env->NewObject(fileClass, fileConstructor, agentPath);

	const jmethodID fileInputStreamConstructor = env->GetMethodID(fileInputStream, "<init>", "(Ljava/io/File;)V");
	const jobject fileInputStreamInstance = env->NewObject(fileInputStream, fileInputStreamConstructor, fileInstance);

	const jclass zipInputStream = env->FindClass("java/util/zip/ZipInputStream");
	const jmethodID zipInputStreamConstructor = env->GetMethodID(zipInputStream, "<init>", "(Ljava/io/InputStream;)V");
	const jobject zipInputStreamInstance = env->NewObject(zipInputStream, zipInputStreamConstructor, fileInputStreamInstance);
	const jmethodID getNextEntry = env->GetMethodID(zipInputStream, "getNextEntry", "()Ljava/util/zip/ZipEntry;");
	const jmethodID close = env->GetMethodID(zipInputStream, "close", "()V");

	const jclass hashMap = env->FindClass("java/util/HashMap");
	const jmethodID hashMapConstructor = env->GetMethodID(hashMap, "<init>", "()V");
	const jmethodID replace = env->GetMethodID(hashMap, "replace", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
	const jmethodID containsKey = env->GetMethodID(hashMap, "containsKey", "(Ljava/lang/Object;)Z");
	const jmethodID put = env->GetMethodID(hashMap, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
	const jmethodID get = env->GetMethodID(hashMap, "get", "(Ljava/lang/Object;)Ljava/lang/Object;");
	const jmethodID values = env->GetMethodID(hashMap, "values", "()Ljava/util/Collection;");
	const jmethodID keySet = env->GetMethodID(hashMap, "keySet", "()Ljava/util/Set;");
	const jobject classes = env->NewObject(hashMap, hashMapConstructor);

	const jclass zipEntry = env->FindClass("java/util/zip/ZipEntry");
	const jmethodID getName = env->GetMethodID(zipEntry, "getName", "()Ljava/lang/String;");
	const jmethodID isDirectory = env->GetMethodID(zipEntry, "isDirectory", "()Z");
	const jmethodID startsWith = env->GetMethodID(stringCls, "startsWith", "(Ljava/lang/String;)Z");

	const jmethodID stringReplace = env->GetMethodID(stringCls, "replace", "(Ljava/lang/CharSequence;Ljava/lang/CharSequence;)Ljava/lang/String;");
	const jmethodID contains = env->GetMethodID(stringCls, "contains", "(Ljava/lang/CharSequence;)Z");

	jobject entry = NULL;
	while ((entry = env->CallObjectMethod(zipInputStreamInstance, getNextEntry)) != NULL) {
		const jstring name = (jstring)env->CallObjectMethod(entry, getName);
		if (!env->CallBooleanMethod(entry, isDirectory) && env->CallBooleanMethod(name, contains, dotString) && !env->CallBooleanMethod(name, startsWith, spaceString) && env->CallBooleanMethod(name, contains, classString)) {
			env->CallVoidMethod(classes, put, (jstring)env->CallObjectMethod(env->CallObjectMethod(name, stringReplace, slashString, dotString), stringReplace, classString, emptyString), toByteArray(env, zipInputStreamInstance)); //env->CallStaticObjectMethod(NULL, toByteArray, zipInputStreamInstance));
			if (env->ExceptionOccurred()) {
				cout << "Exception thrown when loading agent's classes [zip]" << endl;
				env->ExceptionDescribe();
				Sleep(10000L);
			}
		}
	}

	env->CallVoidMethod(zipInputStreamInstance, close);
	return classes;
}

/* end utilities */

static jobject defaultClassLoader = NULL;

void NativeAgent::onAttach(NativeAgent* agent) {
	instance = agent;

	AllocConsole();
	SetConsoleCtrlHandler(NULL, true);
	FILE* fIn;
	FILE* fOut;
	freopen_s(&fIn, "conin$", "r", stdin);
	freopen_s(&fOut, "conout$", "w", stdout);
	freopen_s(&fOut, "conout$", "w", stderr);

	/* definicje */

	const jclass javaClass = env->FindClass("java/lang/Class");
	const jstring path = env->NewStringUTF("C:/agent.jar"); // Lokalizacja agenta
	const jclass loaderClass = env->FindClass("java/lang/ClassLoader");
	const jmethodID loaderMethod = env->GetStaticMethodID(loaderClass, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
	const jobject classLoader = defaultClassLoader = env->NewGlobalRef(env->CallStaticObjectMethod(loaderClass, loaderMethod));

	const jclass stringCls = env->FindClass("java/lang/String");
	const jstring dotString = env->NewStringUTF(".");
	const jstring emptyString = env->NewStringUTF("");
	const jstring spaceString = env->NewStringUTF(" ");
	const jstring classString = env->NewStringUTF(".class");
	const jstring slashString = env->NewStringUTF("/");

	const jclass jarfileClazz = env->FindClass("java/util/jar/JarFile");

	const jmethodID contains = env->GetMethodID(stringCls, "contains", "(Ljava/lang/CharSequence;)Z");
	const jmethodID startsWith = env->GetMethodID(stringCls, "startsWith", "(Ljava/lang/String;)Z");

	const jclass hashMap = env->FindClass("java/util/HashMap");

	const jmethodID stringReplace = env->GetMethodID(stringCls, "replace", "(Ljava/lang/CharSequence;Ljava/lang/CharSequence;)Ljava/lang/String;");
	const jmethodID hashMapConstructor = env->GetMethodID(hashMap, "<init>", "()V");
	const jmethodID replace = env->GetMethodID(hashMap, "replace", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
	const jmethodID containsKey = env->GetMethodID(hashMap, "containsKey", "(Ljava/lang/Object;)Z");
	const jmethodID put = env->GetMethodID(hashMap, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
	const jmethodID get = env->GetMethodID(hashMap, "get", "(Ljava/lang/Object;)Ljava/lang/Object;");
	const jmethodID values = env->GetMethodID(hashMap, "values", "()Ljava/util/Collection;");
	const jmethodID keySet = env->GetMethodID(hashMap, "keySet", "()Ljava/util/Set;");
	const jmethodID remove = env->GetMethodID(hashMap, "remove", "(Ljava/lang/Object;)Ljava/lang/Object;");

	const jclass collection = env->FindClass("java/util/Collection");
	const jmethodID toArray = env->GetMethodID(collection, "toArray", "([Ljava/lang/Object;)[Ljava/lang/Object;");

	const jobject classMap = addClassesToMap(env, path);

	const jobjectArray classNames = (jobjectArray)env->CallObjectMethod(env->CallObjectMethod(classMap, keySet), toArray, env->NewObjectArray(0, stringCls, NULL));
	const jsize classNamesLength = env->GetArrayLength(classNames);

	for (int i = 0; i < classNamesLength; i++) {
		const jstring className = (jstring)env->GetObjectArrayElement(classNames, i);
		const jbyteArray byteArray = (jbyteArray)env->CallObjectMethod(classMap, get, className);
		if (!env->CallBooleanMethod(className, startsWith, env->NewStringUTF("me.hp888.nativeagent.loader.AgentClassLoader")))
			continue;

		env->DefineClass(env->GetStringUTFChars((jstring)env->CallObjectMethod(className, stringReplace, dotString, slashString), JNI_FALSE), classLoader, env->GetByteArrayElements(byteArray, JNI_FALSE), env->GetArrayLength(byteArray));
		cout << "Loaded class (scl) " << env->GetStringUTFChars(className, JNI_FALSE) << endl;
		break;
	}

	const jclass agentClassLoader = env->FindClass("me/hp888/nativeagent/loader/AgentClassLoader");
	const jmethodID agentClassLoaderConstructor = env->GetMethodID(agentClassLoader, "<init>", "(Ljava/util/Map;)V");
	const jobject agentClassLoaderInstance = env->NewObject(agentClassLoader, agentClassLoaderConstructor, classMap);

	const jclass fieldClass = env->FindClass("java/lang/reflect/Field");
	const jclass accessibleObjectClass = env->FindClass("java/lang/reflect/AccessibleObject");

	const jmethodID getDeclaredField = env->GetMethodID(javaClass, "getDeclaredField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;");
	const jmethodID setAccessible = env->GetMethodID(accessibleObjectClass, "setAccessible", "(Z)V");
	const jmethodID set = env->GetMethodID(fieldClass, "set", "(Ljava/lang/Object;Ljava/lang/Object;)V");

	const jobject systemClassLoaderField = env->CallObjectMethod(loaderClass, getDeclaredField, env->NewStringUTF("scl"));
	env->CallVoidMethod(systemClassLoaderField, setAccessible, (jboolean)JNI_TRUE);
	env->CallVoidMethod(systemClassLoaderField, set, (jobject)NULL, agentClassLoaderInstance);
	cout << "[Agent] Changed application's class loader!" << endl;

	const jmethodID loadClass = env->GetMethodID(agentClassLoader, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
	nativeAccesses = (jclass)env->CallObjectMethod(agentClassLoaderInstance, loadClass, env->NewStringUTF("me.hp888.nativeagent.NativeAccesses"));

	reflections = (jclass)env->CallObjectMethod(agentClassLoaderInstance, loadClass, env->NewStringUTF("me.hp888.nativeagent.utils.Reflections"));
	getMethod = env->GetStaticMethodID(reflections, "getMethod", "(Ljava/lang/Class;Ljava/lang/String;)Ljava/lang/reflect/Method;");
	invokeMethod = env->GetStaticMethodID(reflections, "invokeMethod", "(Ljava/lang/reflect/Method;Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");

	env->CallObjectMethod(classMap, remove, env->CallObjectMethod(env->NewStringUTF("me.hp888.nativeagent.loader.AgentClassLoader"), stringReplace, dotString, slashString));

	std::cout << "[Agent] Loaded all classes! Starting main class!" << std::endl;

	// inicjalizacja jvmti

	initializeJvmti(agent);

	// uruchamianie agenta

	const jclass instrumentationImpl = (jclass)env->CallObjectMethod(agentClassLoaderInstance, loadClass, env->NewStringUTF("me.hp888.nativeagent.instrument.impl.InstrumentationImpl")); // Instrumentacja
	const jmethodID instrumentationImplConstructor = env->GetMethodID(instrumentationImpl, "<init>", "()V");
	const jobject instrumentationInstance = env->NewObject(instrumentationImpl, instrumentationImplConstructor);

	const jclass agentMainClass = (jclass)env->CallObjectMethod(agentClassLoaderInstance, loadClass, env->NewStringUTF("me.hp888.nativeagent.AgentBootstrap")); // Klasa glowna agenta
	const jmethodID agentMainMethod = env->GetStaticMethodID(agentMainClass, "startAgent", "(Lme/hp888/nativeagent/instrument/Instrumentation;Ljava/lang/ClassLoader;)V"); // Metoda glowna agenta

	env->CallStaticVoidMethod(agentMainClass, agentMainMethod, instrumentationInstance, agentClassLoaderInstance); // Start agenta

	if (env->ExceptionOccurred()) {
		cout << "[Agent] Exception thrown after launching agent!" << endl;
		env->ExceptionDescribe();
		return;
	}

	// unhook
	//jvm->DetachCurrentThread();
}

jobjectArray asClassArray(JNIEnv* env, jclass* buf, int len) {
	jobjectArray array = env->NewObjectArray(len, env->FindClass("java/lang/Class"), NULL);

	for (int i = 0; i < len; i++) {
		env->SetObjectArrayElement(array, i, buf[i]);
	}

	return array;
}

JNIEXPORT jobjectArray JNICALL Java_me_hp888_nativeagent_instrument_impl_InstrumentationImpl_getAllLoadedClasses(JNIEnv* env, jobject instrumentationInstance) {
	jclass* jvmClasses;
	jint classCount;

	const jint err = instance->jvmti->GetLoadedClasses(&classCount, &jvmClasses);
	if (err) {
		cout << "Unable to get loaded classes at runtime!" << endl;
		return asClassArray(env, jvmClasses, classCount);
	}

	return asClassArray(env, jvmClasses, classCount);
}

JNIEXPORT jobjectArray JNICALL Java_me_hp888_nativeagent_instrument_impl_InstrumentationImpl_getLoadedClasses(JNIEnv* env, jobject instrumentationInstance, jobject classLoader) {
	jclass* jvmClasses;
	jint classCount;

	const jint err = instance->jvmti->GetClassLoaderClasses(classLoader, &classCount, &jvmClasses);
	if (err) {
		cout << "Unable to get loaded classes at runtime!" << endl;
		return asClassArray(env, jvmClasses, classCount);
	}

	return asClassArray(env, jvmClasses, classCount);
}

JNIEXPORT void JNICALL Java_me_hp888_nativeagent_instrument_impl_InstrumentationImpl_retransformClasses(JNIEnv* env, jobject instrumentationInstance, jobjectArray classes) {
	const jclass stringCls = env->FindClass("java/lang/String");
	const jmethodID stringReplace = env->GetMethodID(stringCls, "replace", "(Ljava/lang/CharSequence;Ljava/lang/CharSequence;)Ljava/lang/String;");
	const jstring dotString = env->NewStringUTF(".");
	const jstring slashString = env->NewStringUTF("/");

	const jclass javaClass = env->FindClass("java/lang/Class");
	const jmethodID getName = env->GetMethodID(javaClass, "getName", "()Ljava/lang/String;");

	jint size = env->GetArrayLength(classes);
	jclass* jvmClasses = new jclass[size];
	names = new string[size];

	for (int index = 0; index < size; index++) {
		jvmClasses[index] = (jclass)env->GetObjectArrayElement(classes, index);
		names[index] = env->GetStringUTFChars((jstring)env->CallObjectMethod((jstring)env->CallObjectMethod(jvmClasses[index], getName), stringReplace, dotString, slashString), JNI_FALSE);
	}

	cout << "[Agent] Retransforming " << size << " classes.." << endl;

	loaded = true;
	classesToRedefine = size;
	jvmtiEventCallbacks callbacks = { 0 };
	callbacks.ClassFileLoadHook = classTransformerHook;
	instance->jvmti->SetEventCallbacks(&callbacks, sizeof(callbacks));
	instance->jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, NULL);

	instance->jvmti->RetransformClasses(size, jvmClasses);

	index = 0;
	loaded = false;
	instance->jvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, NULL);
	cout << "[Agent] Retransformed " << size << " classes." << endl;
}