#include <dragonruby.h>
#include <jni.h>

// Global reference to DragonRuby's API
static drb_api_t *drb;
// Global reference to JNIEnv
static JNIEnv *jni_env;

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
  args[1] = drb->mrb_word_boxing_cptr_value(mrb, class);

  return drb->mrb_obj_new(mrb, jni_class, 2, args);
}

DRB_FFI_EXPORT
void drb_register_c_extensions_with_api(mrb_state *mrb, struct drb_api_t *local_drb)
{
  drb = local_drb;
  drb->drb_log_write("Game", 2, "* INFO - Retrieving JNIEnv");
  jni_env = (JNIEnv *)drb->drb_android_get_jni_env();

  struct RClass *jni_module = drb->mrb_module_get(mrb, "JNI");
  drb->mrb_define_class_method(mrb, jni_module, "find_class", jni_find_class, MRB_ARGS_REQ(1));
}
