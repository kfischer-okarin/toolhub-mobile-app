module JNI
  class << self
    # Assigned in jni.c
    attr_reader :game_activity_reference

    def game_activity
      @game_activity ||= JavaObject.new(@game_activity_reference)
    end

    # Defined in jni.c
    # def find_class(name) -> Reference
    # def get_static_method_id(class_reference, name, signature)
    # def get_object_class(object_reference) -> Reference
    # def call_static_boolean_method(class_reference, method_id, *args) -> Boolean
    # def call_static_object_method(class_reference, method_id, *args) -> Reference
  end

  class Exception < StandardError; end
  class ClassNotFound < Exception; end
  class NoSuchMethod < Exception; end

  # Stores a JNI global reference internally
  # Do not use this class directly
  class Reference
    attr_reader :type_name, :qualifier

    def inspect
      "#<JNI::Reference #{@type_name} #{@qualifier}>"
    end
  end

  # Stores a JNI method or field ID internally
  # Do not use this class directly
  class Pointer
    attr_reader :type_name, :qualifier

    def inspect
      "#<JNI::Pointer #{@type_name} #{@qualifier}>"
    end
  end

  class JavaObject
    attr_reader :reference

    def initialize(reference)
      @reference = reference
    end

    def java_class
      @java_class ||= JavaClass.new(JNI.get_object_class(reference))
    end

    def inspect
      "#<JNI::JavaObject #{@reference.qualifier}>"
    end
  end

  class JavaClass
    attr_reader :reference

    def initialize(reference)
      @reference = reference
    end

    def name
      # qualifier has format 'class com.example.MyClass'
      @name ||= @reference.qualifier.split.last
    end

    def inspect
      "#<JNI::JavaClass #{name}>"
    end
  end
end

GTK.dlopen('jni')
