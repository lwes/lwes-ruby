module LWES
  class Struct

    # creates a new Struct-based class from the given ESF +file+
    # with the given +klass_sym+ as the name.  Options may
    # include :event_class (in case it differs in the ESF file)
    # and the :parent_class (Object).
    def self.new(file, klass_sym, options = {}, &block)
      dump = TypeDB.new(file).to_hash
      event_class = options[:event_class] || klass_sym
      parent_class = options[:parent_class] || Object
      event_def = dump[event_class] or
        raise RuntimeError, "#{event_class.inspect} not defined in #{file}"
      event_def = dump[:MetaEventInfo].merge(event_def)
      klass = ::Struct.new(*(event_def.keys))
      klass.const_set :TYPE_DB, event_def.freeze
      klass = parent_class.const_set(klass_sym, klass)

      klass.class_eval(&block) if block_given?
      klass
    end
  end
end
