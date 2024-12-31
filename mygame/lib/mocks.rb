def a_mock(&block)
  result = Mock.new
  result.configure_mock(&block) if block
  result
end

class Mock
  def initialize
    clear_method_calls
  end

  def method_calls(method_name)
    @method_calls[method_name] || []
  end

  def clear_method_calls
    @method_calls = {}
  end

  def configure_mock(&block)
    Configurator.new(self).instance_exec(&block)
  end

  def method_missing(method_name, *args, &block)
    configure_mock do
      responding_to(method_name)
    end
    send(method_name, *args, &block)
  end

  class Configurator
    def initialize(mock)
      @mock = mock
    end

    def responding_to(method_name, &block)
      method_configurator = MethodConfigurator.new(@mock, method_name)
      method_configurator.instance_exec(&block) if block
      method_configurator.define_method
    end
  end

  class MethodConfigurator
    def initialize(mock, method_name)
      @mock = mock
      @method_name = method_name
      @return_values = [nil].cycle
    end

    def always_returning(value)
      @return_values = [value].cycle
    end

    def always_returning_self
      @return_values = [self].cycle
    end

    def returning_values(*values)
      @return_values = values.to_enum
    end

    def define_method
      method_name = @method_name
      return_values = @return_values
      @mock.define_singleton_method(method_name) do |*args, **kwargs|
        @method_calls[method_name] ||= []
        @method_calls[method_name] << [args, kwargs]
        return_values.next
      end
    end
  end
end

module GTK
  class Assert
    def was_called!(mock, method_name, message = nil)
      return ok! unless mock.method_calls(method_name).empty?

      fail_with_message message, "Expected method call #{method_name.inspect}, but it was not called"
    end

    def was_not_called!(mock, method_name, message = nil)
      return ok! if mock.method_calls(method_name).empty?

      fail_with_message message, "Expected not method call #{method_name.inspect}, but it was called"
    end

    def received_call!(mock, method_name, args, kwargs = nil, message = nil)
      kwargs ||= {}
      if args.is_a? Hash
        kwargs = args
        args = []
      end
      message = args if args.is_a? String
      message = kwargs if kwargs.is_a? String

      calls = mock.method_calls(method_name)
      expected_call = [args, kwargs]
      return ok! if calls.include?([args, kwargs])

      fail_with_message message, <<~ERROR
        Expected calls:
        #{safe_format(calls)}

        to include:

        #{expected_call.inspect}
      ERROR
    end
  end
end
