module GTK
  class Assert
    def equal!(actual, expected, message = nil)
      return ok! if actual == expected

      fail_with_message message, <<~ERROR
        actual:
        #{safe_format(actual)}

        did not equal

        expected:
        #{safe_format(expected)}
      ERROR
    end

    def empty!(collection, message = nil)
      return ok! if collection.empty?

      fail_with_message message, <<~ERROR
        Expected:
        #{safe_format(collection)}

        to be empty
      ERROR
    end

    def includes!(collection, element, message = nil)
      return ok! if collection.include?(element)

      fail_with_message message, <<~ERROR
        Expected:
        #{safe_format(collection)}

        to include:

        #{element.inspect}
      ERROR
    end

    def includes_no!(collection, element, message = nil)
      return ok! unless collection.include?(element)

      fail_with_message message, <<~ERROR
        Expected:
        #{safe_format(collection)}

        to not include:

        #{element.inspect}
      ERROR
    end

    def raises!(exception_class, message = nil)
      begin
        yield
        fail_with_message message, "Expected to raise #{exception_class}, but nothing was raised"
      rescue exception_class
        ok!
      rescue Exception => e
        fail_with_message message, "Expected to raise #{exception_class}, but raised: #{e}"
      end
    end

    private

    def fail_with_message(custom_message, base_message)
      fail_message = base_message.trim
      fail_message += "\n\n#{custom_message.trim}" if custom_message
      fail_message += "\n-------------------------"
      raise fail_message
    end

    def safe_format(value)
      pretty_format(value).trim
    rescue SystemStackError
      value.to_s
    end
  end
end
