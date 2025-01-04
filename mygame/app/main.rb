require 'lib/input'

require 'app/jni'

def tick(args)
  # Create an input
  args.state.input ||= Input::Text.new(x: 100, y: 600, w: 300, focussed: true)

  # Allow the input to process inputs and render text (render_target)
  args.state.input.tick

  if args.inputs.mouse.click
    p JNI.game_activity
    activity_class = JNI.game_activity.java_class
    activity_class.register_static_method(:show_text_input, argument_types: %i[int int int int], return_type: :boolean)
    activity_class.show_text_input(100, 100, 300, 100)
  end

  center_x = 360
  args.outputs.labels << {
    x: center_x,
    y: 720 - 32,
    text: 'Click to call JNI method',
    anchor_x: 0.5
  }
  args.outputs.primitives << args.state.input
end
