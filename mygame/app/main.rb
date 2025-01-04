require 'lib/input'

require 'app/jni'

def tick(args)
  # Create an input
  args.state.input ||= Input::Text.new(x: 100, y: 600, w: 300, focussed: true)

  # Allow the input to process inputs and render text (render_target)
  args.state.input.tick

  if args.inputs.mouse.click
    p JNI.game_activity_reference
    activity_class = JNI.get_object_class(JNI.game_activity_reference)
    p activity_class
    method_id = JNI.get_static_method_id(activity_class, 'showTextInput', '(IIII)Z')
    p method_id
    result = JNI.call_static_boolean_method(activity_class, method_id, 100, 200, 300, 400)
    p result
  end

  center_x = 360
  args.outputs.labels << {
    x: center_x,
    y: 720 - 32,
    text: 'Click to call JNI method',
    anchor_x: 0.5
  }
  args.outputs.primitives << args.state.input
  # args.outputs.labels << {
    # x: center_x,
    # y: 720 - 64,
    # text: "Last Input: #{args.inputs.keyboard.key_held.d}",
    # anchor_x: 0.5
  # }
end
