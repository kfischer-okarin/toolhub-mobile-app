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
  struct RClass *jni_pointer;
  struct RClass *jni_exception;
};

static struct references refs;

// ----- JNI Debugging Helpers -----

static void print_last_jni_exception() {
  if ((*jni_env)->ExceptionCheck(jni_env)) {
    (*jni_env)->ExceptionDescribe(jni_env);
    (*jni_env)->ExceptionClear(jni_env);
  }
}

static void drb_log_writef(const char *format, ...) {
  va_list args;
  va_start(args, format);
  char buffer[1000];
  vsnprintf(buffer, 1000, format, args);
  drb->drb_log_write("Game", 2, buffer);
  va_end(args);
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

// ----- JNI Pointer Data Type -----

static mrb_value wrap_jni_pointer_in_object(mrb_state *mrb, void *pointer, const char *type_name, mrb_value qualifier) {
  mrb_value pointer_value = drb->mrb_word_boxing_cptr_value(mrb, pointer);
  mrb_value result = drb->mrb_obj_new(mrb, refs.jni_pointer, 0, NULL);
  drb->mrb_iv_set(mrb, result, drb->mrb_intern_lit(mrb, "@pointer"), pointer_value);
  drb->mrb_iv_set(mrb, result, drb->mrb_intern_lit(mrb, "@type_name"), drb->mrb_str_new_cstr(mrb, type_name));
  drb->mrb_iv_set(mrb, result, drb->mrb_intern_lit(mrb, "@qualifier"), qualifier);
  return result;
}

static void *unwrap_jni_pointer_from_object(mrb_state *mrb, mrb_value object) {
  mrb_value pointer_value = drb->mrb_iv_get(mrb, object, drb->mrb_intern_lit(mrb, "@pointer"));
  return mrb_cptr(pointer_value);
}

// ----- JNI Pointer Data Type END -----

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
  struct RClass *exception_class = refs.jni_exception;

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

// ----- JNI Methods -----

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

  return wrap_jni_pointer_in_object(mrb, method_id, "jmethodID", qualifier);
}

static mrb_value jni_get_object_class_m(mrb_state *mrb, mrb_value self) {
  mrb_value object_reference;
  drb->mrb_get_args(mrb, "o", &object_reference);

  jobject object = drb->mrb_data_check_get_ptr(mrb, object_reference, &jni_reference_data_type);
  jclass class = (*jni_env)->GetObjectClass(jni_env, object);
  handle_jni_exception(mrb);

  return wrap_jni_reference_in_object(mrb,
                                      class,
                                      "jclass",
                                      drb->mrb_str_new_cstr(mrb, java_object_to_string(class)));
}

static jvalue *convert_mrb_args_to_jni_args(mrb_state *mrb, mrb_value *args, mrb_int argc) {
  jvalue *jni_args = drb->mrb_malloc(mrb, sizeof(jvalue) * argc);
  for (int i = 0; i < argc; i++) {
    if (mrb_integer_p(args[i])) {
      jni_args[i].i = mrb_integer(args[i]);
    } else {
      drb->mrb_raise(mrb, refs.jni_exception, "Only Fixnum arguments are supported");
    }
  }
  return jni_args;
}

#define CALL_STATIC_METHOD_BEGINNING()\
  mrb_value class_reference;\
  mrb_value method_id_reference;\
  mrb_value *args;\
  mrb_int argc;\
  drb->mrb_get_args(mrb, "oo*", &class_reference, &method_id_reference, &args, &argc);\
  \
  jclass class = drb->mrb_data_check_get_ptr(mrb, class_reference, &jni_reference_data_type);\
  jmethodID method_id = (jmethodID)unwrap_jni_pointer_from_object(mrb, method_id_reference);\
  \
  jvalue *jni_args = convert_mrb_args_to_jni_args(mrb, args, argc);

#define CALL_STATIC_METHOD_CLEANUP()\
  drb->mrb_free(mrb, jni_args);\
  handle_jni_exception(mrb);

static mrb_value jni_call_static_boolean_method_m(mrb_state *mrb, mrb_value self) {
  CALL_STATIC_METHOD_BEGINNING();

  jboolean jni_result = (*jni_env)->CallStaticBooleanMethodA(jni_env, class, method_id, jni_args);

  CALL_STATIC_METHOD_CLEANUP();

  return mrb_bool_value(jni_result);
}

static mrb_value jni_call_static_object_method_m(mrb_state *mrb, mrb_value self) {
  CALL_STATIC_METHOD_BEGINNING();

  jobject jni_result = (*jni_env)->CallStaticObjectMethodA(jni_env, class, method_id, jni_args);

  CALL_STATIC_METHOD_CLEANUP();

  return wrap_jni_reference_in_object(mrb,
                                      jni_result,
                                      "jobject",
                                      drb->mrb_str_new_cstr(mrb, java_object_to_string(jni_result)));
}

// ----- JNI Methods END -----

DRB_FFI_EXPORT
void drb_register_c_extensions_with_api(mrb_state *mrb, struct drb_api_t *local_drb) {
  drb = local_drb;
  drb->drb_log_write("Game", 2, "* INFO - Retrieving JNIEnv");
  jni_env = (JNIEnv *)drb->drb_android_get_jni_env();

  refs.jni = drb->mrb_module_get_under(mrb, drb->mrb_module_get(mrb, "JNI"), "FFI");
  refs.jni_pointer = drb->mrb_class_get_under(mrb, refs.jni, "Pointer");
  refs.jni_reference = drb->mrb_class_get_under(mrb, refs.jni, "Reference");
  refs.jni_exception = drb->mrb_class_get_under(mrb, refs.jni, "Exception");
  MRB_SET_INSTANCE_TT(refs.jni_reference, MRB_TT_DATA);

  drb->mrb_define_class_method(mrb, refs.jni, "find_class", jni_find_class_m, MRB_ARGS_REQ(1));
  drb->mrb_define_class_method(mrb, refs.jni, "get_object_class", jni_get_object_class_m, MRB_ARGS_REQ(1));
  drb->mrb_define_class_method(mrb, refs.jni, "get_static_method_id", jni_get_static_method_id_m, MRB_ARGS_REQ(3));
  drb->mrb_define_class_method(mrb,
                               refs.jni,
                               "call_static_boolean_method",
                               jni_call_static_boolean_method_m,
                               MRB_ARGS_REQ(2) | MRB_ARGS_REST());
  drb->mrb_define_class_method(mrb,
                               refs.jni,
                               "call_static_object_method",
                               jni_call_static_object_method_m,
                               MRB_ARGS_REQ(2) | MRB_ARGS_REST());

  jobject activity = (jobject) drb->drb_android_get_sdl_activity();
  drb->mrb_iv_set(mrb,
                  drb->mrb_obj_value(refs.jni),
                  drb->mrb_intern_lit(mrb, "@game_activity_reference"),
                  wrap_jni_reference_in_object(mrb,
                                               activity,
                                               "jobject",
                                               drb->mrb_str_new_cstr(mrb, java_object_to_string(activity))));
}
