module JNI
  class << self
    # Set via C extension
    attr_reader :game_activity_reference

    # Defined in jni.c
    # def find_class(name) -> Reference
    # def get_static_method_id(class_reference, name, signature)
    # def get_object_class(object_reference) -> Reference
  end

  class Exception < StandardError; end
  class ClassNotFound < Exception; end
  class NoSuchMethod < Exception; end

  class Reference
    attr_reader :type_name, :qualifier

    def inspect
      "#<JNI::Reference #{@type_name} #{@qualifier}>"
    end
  end
end

GTK.dlopen('jni')
