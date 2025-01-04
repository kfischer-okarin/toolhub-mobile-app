require_relative 'jni/ffi'

module JNI
  class << self
    def game_activity
      @game_activity ||= JavaObject.new(FFI.game_activity_reference)
    end
  end

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

    def name
      # qualifier has format 'class com.example.MyClass'
      @name ||= @reference.qualifier.split.last
    end

    def inspect
      "#<#{self.class} #{name}>"
    end
  end
end

