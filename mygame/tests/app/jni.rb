require 'tests/test_helper'

module JNI
  describe JavaObject do
    it 'can retrieve its JavaClass' do
      ffi = a_mock {
        responding_to(:get_object_class) {
          always_returning({ qualifier: 'class com.example.MyObject' })
        }
      }
      object_reference = Object.new
      object = JavaObject.new(object_reference, ffi: ffi)

      result = object.java_class

      assert.equal! result.name, 'com.example.MyObject'
      assert.received_call! ffi, :get_object_class, [object_reference]
    end
  end
end
