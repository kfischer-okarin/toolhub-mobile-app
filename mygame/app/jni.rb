module JNI
  class << self
    # Set via C extension
    attr_reader :game_activity

    # Defined in jni.c
    # def find_class(name)
  end

  class Exception < StandardError; end
  class ClassNotFound < Exception; end

  class Reference
    def inspect
      "#<JNI::Reference '#{@description}'>"
    end
  end
end

GTK.dlopen('jni')
