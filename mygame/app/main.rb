require 'app/jni'

def tick(args)
  if args.inputs.mouse.click
    p JNI.game_activity_reference
    activity_class = JNI.get_object_class(JNI.game_activity_reference)
    p activity_class
    method_id = JNI.get_static_method_id(activity_class, 'showTextInput', '(IIII)Z')
    p method_id
    result = JNI.call_static_boolean_method(activity_class, method_id, 100, 200, 300, 400)
    p result
  end

  if args.inputs.keyboard.key_down.enter
    puts "Hello from Ruby!"
  end
end
