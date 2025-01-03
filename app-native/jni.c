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
  struct RClass *jni_object;
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

static mrb_value wrap_jni_reference_in_object(mrb_state *mrb, jobject reference, struct RClass *klass) {
  jobject global_reference = (*jni_env)->NewGlobalRef(jni_env, reference);
  struct RData *data = drb->mrb_data_object_alloc(mrb, klass, global_reference, &jni_reference_data_type);
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

static mrb_value wrap_java_class(mrb_state *mrb, jclass class) {
  mrb_value result = wrap_jni_reference_in_object(mrb, class, refs.jni_class);

  mrb_value name = drb->mrb_str_new_cstr(mrb, get_java_class_name(class));
  drb->mrb_iv_set(mrb, result, drb->mrb_intern_lit(mrb, "@name"), name);

  return result;
}

static mrb_value wrap_java_object(mrb_state *mrb, jobject object) {
  mrb_value result = wrap_jni_reference_in_object(mrb, object, refs.jni_object);

  jclass object_class = (*jni_env)->GetObjectClass(jni_env, object);
  drb->mrb_iv_set(mrb,
                  result,
                  drb->mrb_intern_lit(mrb, "@java_class"),
                  wrap_java_class(mrb, object_class));

  return result;
}

static mrb_value jni_find_class(mrb_state *mrb, mrb_value self) {
  const char *class_name;
  drb->mrb_get_args(mrb, "z", &class_name);

  jclass class = (*jni_env)->FindClass(jni_env, class_name);
  handle_jni_exception(mrb);

  return wrap_java_class(mrb, class);
}

DRB_FFI_EXPORT
void drb_register_c_extensions_with_api(mrb_state *mrb, struct drb_api_t *local_drb) {
  drb = local_drb;
  drb->drb_log_write("Game", 2, "* INFO - Retrieving JNIEnv");
  jni_env = (JNIEnv *)drb->drb_android_get_jni_env();

  refs.jni = drb->mrb_module_get(mrb, "JNI");
  refs.jni_class = drb->mrb_class_get_under(mrb, refs.jni, "JavaClass");
  MRB_SET_INSTANCE_TT(refs.jni_class, MRB_TT_DATA);
  refs.jni_object = drb->mrb_class_get_under(mrb, refs.jni, "JavaObject");
  MRB_SET_INSTANCE_TT(refs.jni_object, MRB_TT_DATA);

  drb->mrb_define_class_method(mrb, refs.jni, "find_class", jni_find_class, MRB_ARGS_REQ(1));

  jobject activity = (jobject) drb->drb_android_get_sdl_activity();
  drb->mrb_iv_set(mrb,
                  drb->mrb_obj_value(refs.jni),
                  drb->mrb_intern_lit(mrb, "@game_activity"),
                  wrap_java_object(mrb, activity));
}
