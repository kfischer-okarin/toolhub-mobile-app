#include <stdbool.h>
#include <dragonruby.h>
#include <jni.h>

// Global reference to DragonRuby's API
static drb_api_t *drb;
// Global reference to JNIEnv
static JNIEnv *jni_env;

struct references {
  struct RClass *jni;
  struct RClass *jni_class;
  struct RClass *jni_reference;
};

static struct references refs;

// ----- JNI Debugging Helpers -----

static void print_last_jni_exception() {
  if ((*jni_env)->ExceptionCheck(jni_env)) {
    (*jni_env)->ExceptionDescribe(jni_env);
    (*jni_env)->ExceptionClear(jni_env);
  }
}

// ----- JNI Reference Data Type -----

static void jni_reference_free(mrb_state *mrb, void *ptr) {
  (*jni_env)->DeleteGlobalRef(jni_env, ptr);
}

static const mrb_data_type jni_reference_data_type = {
    "JNI::Reference",
    jni_reference_free,
};

static mrb_value wrap_jni_reference_in_object(mrb_state *mrb, jobject reference) {
  jobject global_reference = (*jni_env)->NewGlobalRef(jni_env, reference);
  struct RData *data = drb->mrb_data_object_alloc(mrb, refs.jni_reference, global_reference, &jni_reference_data_type);
  return drb->mrb_obj_value(data);
}

// ----- JNI Reference Data Type END -----

static char *get_java_class_name(jclass class) {
  jclass class_class = (*jni_env)->FindClass(jni_env, "java/lang/Class");
  jmethodID get_name_method = (*jni_env)->GetMethodID(jni_env, class_class, "getName", "()Ljava/lang/String;");
  jstring name = (jstring)(*jni_env)->CallObjectMethod(jni_env, class, get_name_method);
  return (char *)(*jni_env)->GetStringUTFChars(jni_env, name, NULL);
}

static bool java_object_is_instance_of(jobject object, const char *class_name) {
  jclass class = (*jni_env)->FindClass(jni_env, class_name);
  return (*jni_env)->IsInstanceOf(jni_env, object, class);
}

static void handle_jni_exception(mrb_state *mrb) {
  jthrowable exception = (*jni_env)->ExceptionOccurred(jni_env);
  if (exception == NULL) {
    return;
  }
  (*jni_env)->ExceptionClear(jni_env);

  // get Message
  jclass exception_class = (*jni_env)->GetObjectClass(jni_env, exception);
  jmethodID get_message_method = (*jni_env)->GetMethodID(jni_env, exception_class, "getMessage", "()Ljava/lang/String;");
  jstring message = (jstring)(*jni_env)->CallObjectMethod(jni_env, exception, get_message_method);
  const char *message_str = (char *)(*jni_env)->GetStringUTFChars(jni_env, message, NULL);

  if (java_object_is_instance_of(exception, "java/lang/ClassNotFoundException")) {
    struct RClass *jni_class_not_found_exception = drb->mrb_class_get_under(mrb, refs.jni_class, "NotFound");
    drb->mrb_raisef(mrb, jni_class_not_found_exception, "Class not found: %s", message_str);
  }

  drb->drb_log_write("Game", 2, "Unhandled JNI Exception:");
  drb->drb_log_write("Game", 2, get_java_class_name(exception_class));
  drb->drb_log_write("Game", 2, message_str);
}

static mrb_value jni_find_class(mrb_state *mrb, mrb_value self) {
  const char *class_name;
  drb->mrb_get_args(mrb, "z", &class_name);

  jclass class = (*jni_env)->FindClass(jni_env, class_name);

  handle_jni_exception(mrb);

  mrb_value args[2];
  args[0] = drb->mrb_str_new_cstr(mrb, class_name);
  args[1] = wrap_jni_reference_in_object(mrb, class);

  return drb->mrb_obj_new(mrb, refs.jni_class, 2, args);
}

DRB_FFI_EXPORT
void drb_register_c_extensions_with_api(mrb_state *mrb, struct drb_api_t *local_drb) {
  drb = local_drb;
  drb->drb_log_write("Game", 2, "* INFO - Retrieving JNIEnv");
  jni_env = (JNIEnv *)drb->drb_android_get_jni_env();

  refs.jni = drb->mrb_module_get(mrb, "JNI");
  refs.jni_class = drb->mrb_class_get_under(mrb, refs.jni, "JavaClass");

  refs.jni_reference = drb->mrb_define_class_under(mrb, refs.jni, "Reference", mrb->object_class);
  MRB_SET_INSTANCE_TT(refs.jni_reference, MRB_TT_DATA);

  drb->mrb_define_class_method(mrb, refs.jni, "find_class", jni_find_class, MRB_ARGS_REQ(1));
}
