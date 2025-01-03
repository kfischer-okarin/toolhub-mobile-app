require 'app/jni'

def tick(args)
  if args.inputs.mouse.click
    string_class = JNI.find_class('java/lang/String')
    p string_class
    p JNI.get_static_method_id(string_class, 'valueOf', '(I)Ljava/lang/String;')
    # p JNI.game_activity.java_class.static_method('showTextInput', '(IIII)Z')
  end
end
