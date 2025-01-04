require 'tests/test_helper'

describe JNI do
  describe 'can calculate a method signature' do
    [
      [[], :void, '()V'],
      [%i[int boolean], :boolean, '(IZ)Z']
    ].each do |argument_types, return_type, expected|
      it "for #{argument_types} -> #{return_type}" do
        result = JNI.method_signature(argument_types, return_type)
        assert.equal! result, expected
      end
    end
  end
end

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

  describe JavaClass do
    it 'can register a static method' do
      ffi = a_mock {
        responding_to(:get_static_method_id) {
          always_returning(1234)
        }
        responding_to(:call_static_boolean_method) {
          always_returning(true)
        }
      }
      class_reference = { qualifier: 'class com.example.MyClass' }
      java_class = JavaClass.new(class_reference, ffi: ffi)

      java_class.register_static_method(:my_method, argument_types: %i[int boolean], return_type: :boolean)

      assert.received_call! ffi, :get_static_method_id, [class_reference, 'myMethod', '(IZ)Z']

      result = java_class.my_method(1, true)

      assert.received_call! ffi, :call_static_boolean_method, [class_reference, 1234, 1, true]
      assert.equal! result, true
    end
  end
end
