module JNI
  class << self
    # Set via C extension
    attr_reader :game_activity
  end

  class JavaObject
    # Set via C extension
    attr_reader :java_class

    def inspect
      "#<JavaObject java_class=#{java_class.inspect}>"
    end
  end

  class JavaClass
    class NotFound < StandardError; end

    # Set via C extension
    attr_reader :name

    def inspect
      "#<JavaClass name=#{name.inspect}>"
    end
  end
end

GTK.dlopen('jni')
