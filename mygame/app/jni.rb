module JNI
  class << self
    attr_reader :game_activity
  end

  class JavaObject
  end

  class JavaClass
    class NotFound < StandardError; end

    def initialize(name, jni_class_pointer)
      @name = name
      @jni_class_pointer = jni_class_pointer
    end

    def to_s
      "JNI::JavaClass(#{@name})"
    end

    alias inspect to_s
  end
end

GTK.dlopen('jni')
