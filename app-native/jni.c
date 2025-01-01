#include <dragonruby.h>
#include <jni.h>

// Global reference to DragonRuby's API
static drb_api_t *drb;
// Global reference to JNIEnv
static JNIEnv *jni_env;

DRB_FFI_EXPORT
void drb_register_c_extensions_with_api(mrb_state *mrb, struct drb_api_t *local_drb)
{
  drb = local_drb;
  drb->drb_log_write("Game", 2, "* INFO - Retrieving JNIEnv");
  jni_env = (JNIEnv *)drb->drb_android_get_jni_env();

  struct RClass *jni_module = drb->mrb_define_module(mrb, "JNI");
}
