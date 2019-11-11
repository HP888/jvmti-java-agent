#pragma once

#include "main.h"

class NativeAgent
{
	public:
		/* functions */

		NativeAgent();
		void onAttach(NativeAgent* agent);

		/* variables */

		jvmtiEnv* jvmti;
		JavaVM* jvm;
		JNIEnv* env;
};