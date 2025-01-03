#include <dragonruby.h>
#include <jni.h>

// Global reference to DragonRuby's API
static drb_api_t *drb;
// Global reference to JNIEnv
static JNIEnv *jni_env;


// ----- JNI Reference Data Type -----

static struct RClass *jni_reference_class;

static void jni_reference_free(mrb_state *mrb, void *ptr)
{
  (*jni_env)->DeleteGlobalRef(jni_env, ptr);
}

static const mrb_data_type jni_reference_data_type = {
    "JNI::Reference",
    jni_reference_free,
};

static mrb_value wrap_jni_reference_in_object(mrb_state *mrb, jobject reference)
{
  jobject global_reference = (*jni_env)->NewGlobalRef(jni_env, reference);
  struct RData *data = drb->mrb_data_object_alloc(mrb, jni_reference_class, global_reference, &jni_reference_data_type);
  return drb->mrb_obj_value(data);
}

// ----- JNI Reference Data Type END -----

static mrb_value jni_find_class(mrb_state *mrb, mrb_value self)
{
  const char *class_name;
  drb->mrb_get_args(mrb, "z", &class_name);

  jclass class = (*jni_env)->FindClass(jni_env, class_name);
  if (class == NULL)
  {
    return drb->mrb_nil_value();
  }

  struct RClass *jni_module = drb->mrb_module_get(mrb, "JNI");
  struct RClass *jni_class = drb->mrb_class_get_under(mrb, jni_module, "Class");

  mrb_value args[2];
  args[0] = drb->mrb_str_new_cstr(mrb, class_name);
  args[1] = wrap_jni_reference_in_object(mrb, class);

  return drb->mrb_obj_new(mrb, jni_class, 2, args);
}

DRB_FFI_EXPORT
void drb_register_c_extensions_with_api(mrb_state *mrb, struct drb_api_t *local_drb)
{
  drb = local_drb;
  drb->drb_log_write("Game", 2, "* INFO - Retrieving JNIEnv");
  jni_env = (JNIEnv *)drb->drb_android_get_jni_env();

  struct RClass *jni_module = drb->mrb_module_get(mrb, "JNI");

  jni_reference_class = drb->mrb_define_class_under(mrb, jni_module, "Reference", mrb->object_class);
  MRB_SET_INSTANCE_TT(jni_reference_class, MRB_TT_DATA);

  drb->mrb_define_class_method(mrb, jni_module, "find_class", jni_find_class, MRB_ARGS_REQ(1));
}
