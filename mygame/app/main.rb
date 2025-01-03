require 'app/jni'

def tick(args)
  if args.inputs.mouse.click
    p JNI.find_class('java/lang/String')
    # p JNI.game_activity.java_class.static_method('showTextInput', '(IIII)Z')
  end
end
