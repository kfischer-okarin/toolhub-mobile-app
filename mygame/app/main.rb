require 'app/jni'

def tick(args)
  if args.inputs.mouse.click
    p JNI.find_class('java/lang/String')
    p JNI.game_activity
    # p JNI.find_class('dev/kf_labo/toolhub/DragonRubyActivity')
  end
end
