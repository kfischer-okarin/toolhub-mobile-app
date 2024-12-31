def describe(name, focus: false, &block)
  Spec::ExampleGroup.new(name, focus: focus, &block)
end

def fdescribe(name, &block)
  describe(name, focus: true, &block)
end

def xdescribe(_name, &block); end

module Spec
  class ExampleGroup
    def initialize(name, focus: false, before_each_blocks: nil, helpers: nil, &block)
      @name = name
      @focus = focus
      @before_each_blocks = before_each_blocks || []
      @helpers = helpers || {}
      if name.is_a? Module
        @name = name.name
        subject { name }
      end

      # Collect child groups and examples and define afterwards so all helpers and before_each blocks are collected
      @child_groups = []

      instance_exec(&block)

      @child_groups.each do |child_group|
        ExampleGroup.new(
          child_group[:name],
          focus: child_group[:focus],
          before_each_blocks: @before_each_blocks.dup,
          helpers: @helpers.dup,
          &child_group[:block]
        )
      end
    end

    def describe(name, focus: @focus, &block)
      @child_groups << {
        name: "#{@name} - #{name}",
        focus: focus,
        block: block
      }
    end

    alias context describe

    def fdescribe(name, &block)
      describe(name, focus: true, &block)
    end

    alias fcontext fdescribe

    def xdescribe(name, &block); end

    alias xcontext xdescribe

    def it(name, focus: @focus, &block)
      test_prefix = focus ? 'focus_test' : 'test'
      before_each_blocks = @before_each_blocks
      helpers = @helpers
      Object.define_method("#{test_prefix}_#{@name} - #{name}") do |args, assert|
        example = Example.new(args, assert)
        helpers.each do |helper_name, helper_block|
          example.define_singleton_method(helper_name, &helper_block)
        end
        before_each_blocks.each do |before_each_block|
          example.instance_exec(&before_each_block)
        end
        example.instance_exec(&block)
      end
    end

    alias test it
    alias they it

    def fit(name, &block)
      it(name, focus: true, &block)
    end

    alias ftest fit
    alias fthey fit

    def xit(_name, &block); end

    alias xtest xit
    alias xthey xit

    def before_each(&block)
      @before_each_blocks << block
    end

    def subject(&block)
      let(:subject, &block)
    end

    def tested_method(method_name)
      define_helper :call_with do |*args, **kwargs|
        # mruby cannot do proper args/kwargs forwarding so check manually
        # Fix this after mruby 3.2
        if args.size.positive? && kwargs.size.positive?
          subject.send(method_name, *args, **kwargs)
        elsif args.size.positive?
          subject.send(method_name, *args)
        elsif kwargs.size.positive?
          subject.send(method_name, **kwargs)
        else
          subject.send(method_name)
        end
      end
    end

    def let(name, &block)
      define_helper name do
        @let_cache ||= {}
        @let_cache[name] ||= instance_exec(&block)
      end
    end

    def define_helper(name, &block)
      @helpers[name] = block
    end
  end

  class Example
    attr_reader :args, :assert

    def initialize(args, assert)
      @args = args
      @assert = assert
    end
  end
end
