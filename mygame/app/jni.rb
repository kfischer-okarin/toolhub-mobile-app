module JNI
  class Class
    class NotFound < StandardError; end

    def initialize(name, jni_class_pointer)
      @name = name
      @jni_class_pointer = jni_class_pointer
    end

    def to_s
      "JNI::Class(#{@name})"
    end

    alias inspect to_s
  end
end

GTK.dlopen('jni')
