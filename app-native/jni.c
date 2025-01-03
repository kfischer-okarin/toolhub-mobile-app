#include <stdbool.h>
#include <dragonruby.h>
#include <jni.h>

// Global reference to DragonRuby's API
static drb_api_t *drb;
// Global reference to JNIEnv
static JNIEnv *jni_env;

struct references {
  struct RClass *jni;
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

static mrb_value wrap_jni_reference_in_object(mrb_state *mrb,
                                              jobject reference,
                                              const char *type_name,
                                              mrb_value qualifier) {
  jobject global_reference = (*jni_env)->NewGlobalRef(jni_env, reference);
  struct RData *data = drb->mrb_data_object_alloc(mrb, refs.jni_reference, global_reference, &jni_reference_data_type);
  mrb_value result = drb->mrb_obj_value(data);
  drb->mrb_iv_set(mrb, result, drb->mrb_intern_lit(mrb, "@type_name"), drb->mrb_str_new_cstr(mrb, type_name));
  drb->mrb_iv_set(mrb, result, drb->mrb_intern_lit(mrb, "@qualifier"), qualifier);
  return result;
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

static const char *get_exception_message(jthrowable exception) {
  jclass exception_class = (*jni_env)->GetObjectClass(jni_env, exception);
  jmethodID get_message_method = (*jni_env)->GetMethodID(jni_env, exception_class, "getMessage", "()Ljava/lang/String;");
  jstring message = (jstring)(*jni_env)->CallObjectMethod(jni_env, exception, get_message_method);
  return (char *)(*jni_env)->GetStringUTFChars(jni_env, message, NULL);
}

static void handle_jni_exception(mrb_state *mrb) {
  jthrowable exception = (*jni_env)->ExceptionOccurred(jni_env);
  if (exception == NULL) {
    return;
  }
  (*jni_env)->ExceptionClear(jni_env);

  const char *message = get_exception_message(exception);
  struct RClass *exception_class = drb->mrb_class_get_under(mrb, refs.jni, "Exception");

  if (java_object_is_instance_of(exception, "java/lang/ClassNotFoundException")) {
    exception_class = drb->mrb_class_get_under(mrb, refs.jni, "ClassNotFound");
  } else if (java_object_is_instance_of(exception, "java/lang/NoSuchMethodError")) {
    exception_class = drb->mrb_class_get_under(mrb, refs.jni, "NoSuchMethod");
  }

  drb->mrb_raise(mrb, exception_class, message);
}

static const char *java_object_to_string(jobject object) {
  jclass class = (*jni_env)->GetObjectClass(jni_env, object);
  jmethodID to_string_method = (*jni_env)->GetMethodID(jni_env, class, "toString", "()Ljava/lang/String;");
  jstring string = (jstring)(*jni_env)->CallObjectMethod(jni_env, object, to_string_method);
  return (char *)(*jni_env)->GetStringUTFChars(jni_env, string, NULL);
}

static mrb_value jni_find_class_m(mrb_state *mrb, mrb_value self) {
  const char *class_name;
  drb->mrb_get_args(mrb, "z", &class_name);

  jclass class = (*jni_env)->FindClass(jni_env, class_name);
  handle_jni_exception(mrb);

  return wrap_jni_reference_in_object(mrb,
                                      class,
                                      "jclass",
                                      drb->mrb_str_new_cstr(mrb, java_object_to_string(class)));
}

static mrb_value jni_get_static_method_id_m(mrb_state *mrb, mrb_value self) {
  mrb_value class_reference;
  const char *method_name;
  const char *method_signature;
  drb->mrb_get_args(mrb, "ozz", &class_reference, &method_name, &method_signature);

  jclass class = drb->mrb_data_check_get_ptr(mrb, class_reference, &jni_reference_data_type);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, class, method_name, method_signature);
  handle_jni_exception(mrb);

  mrb_value qualifier = drb->mrb_iv_get(mrb, class_reference, drb->mrb_intern_lit(mrb, "@qualifier"));
  qualifier = drb->mrb_str_cat_cstr(mrb, qualifier, " ");
  qualifier = drb->mrb_str_cat_cstr(mrb, qualifier, method_name);
  qualifier = drb->mrb_str_cat_cstr(mrb, qualifier, method_signature);

  return wrap_jni_reference_in_object(mrb, method_id, "jmethodID", qualifier);
}

DRB_FFI_EXPORT
void drb_register_c_extensions_with_api(mrb_state *mrb, struct drb_api_t *local_drb) {
  drb = local_drb;
  drb->drb_log_write("Game", 2, "* INFO - Retrieving JNIEnv");
  jni_env = (JNIEnv *)drb->drb_android_get_jni_env();

  refs.jni = drb->mrb_module_get(mrb, "JNI");
  refs.jni_reference = drb->mrb_class_get_under(mrb, refs.jni, "Reference");
  MRB_SET_INSTANCE_TT(refs.jni_reference, MRB_TT_DATA);

  drb->mrb_define_class_method(mrb, refs.jni, "find_class", jni_find_class_m, MRB_ARGS_REQ(1));
  drb->mrb_define_class_method(mrb, refs.jni, "get_static_method_id", jni_get_static_method_id_m, MRB_ARGS_REQ(3));

  /* jobject activity = (jobject) drb->drb_android_get_sdl_activity(); */
  /* drb->mrb_iv_set(mrb, */
                  /* drb->mrb_obj_value(refs.jni), */
                  /* drb->mrb_intern_lit(mrb, "@game_activity"), */
                  /* wrap_java_object(mrb, activity)); */
}
