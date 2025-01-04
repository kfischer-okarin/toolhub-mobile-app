module JNI
  module FFI
    class << self
      attr_reader :game_activity_reference

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
        "#<#{self.class.name} #{@type_name} #{@qualifier}>"
      end
    end

    # Stores a JNI method or field ID internally
    # Do not use this class directly
    class Pointer
      attr_reader :type_name, :qualifier

      def inspect
        "#<#{self.class.name} #{@type_name} #{@qualifier}>"
      end
    end
  end
end

GTK.dlopen('jni')
