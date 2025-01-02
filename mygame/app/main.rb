require 'app/jni'

def tick(args)
  if args.inputs.mouse.click
    p JNI.find_class('java/lang/String')
    p JNI.find_class('java/lang/Stdddring')
  end
end
