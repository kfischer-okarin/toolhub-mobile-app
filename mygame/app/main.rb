require 'app/jni'

def tick(args)
  if args.inputs.mouse.click
    begin
      p JNI.find_class('java/lang/String')
      p JNI.find_class('java/lang/Stdddring')
    rescue JNI::Class::NotFound => e
      p [e.class, e.message]
    end
  end
end
