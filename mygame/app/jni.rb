require_relative 'jni/ffi'

module JNI
  class << self
    def game_activity
      @game_activity ||= JavaObject.new(FFI.game_activity_reference)
    end

    def snake_case_to_camel_case(snake_case)
      parts = snake_case.to_s.split('_')
      [parts[0], *parts[1..].map(&:capitalize)].join
    end

    def method_signature(argument_types, return_type)
      argument_types = argument_types.map { |arg| type_signature(arg) }.join
      return_type = type_signature(return_type)
      "(#{argument_types})#{return_type}"
    end

    def type_signature(type)
      result = TYPE_SIGNATURES[type]
      raise "Unknown type: #{type}" unless result

      result
    end
  end

  TYPE_SIGNATURES = {
    boolean: 'Z',
    int: 'I',
    void: 'V'
  }

  class JavaObject
    attr_reader :reference

    def initialize(reference, ffi: FFI)
      @reference = reference
      @ffi = ffi
    end

    def java_class
      @java_class ||= JavaClass.new(@ffi.get_object_class(reference), ffi: @ffi)
    end

    def inspect
      "#<#{self.class} #{@reference.qualifier}>"
    end
  end

  class JavaClass
    attr_reader :reference

    def initialize(reference, ffi: FFI)
      @reference = reference
      @ffi = ffi
    end

    def register_static_method(name, argument_types: [], return_type: :void)
      method_name = JNI.snake_case_to_camel_case(name)
      signature = JNI.method_signature(argument_types, return_type)
      method_id = @ffi.get_static_method_id(@reference, method_name, signature)
      define_singleton_method name do |*args|
        @ffi.call_static_boolean_method(@reference, method_id, *args)
      end
    end

    def name
      # qualifier has format 'class com.example.MyClass'
      @name ||= @reference.qualifier.split.last
    end

    def inspect
      "#<#{self.class} #{name}>"
    end
  end
end

