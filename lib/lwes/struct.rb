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
      meta_event_info = dump[:MetaEventInfo]
      alpha = proc { |a,b| a.first.to_s <=> b.first.to_s }
      event_def = event_def.sort(&alpha)
      if meta_event_info
        seen = event_def.map { |(field, _)| field }
        meta_event_info.sort(&alpha).each do |field_type|
          seen.include?(field_type.first) or event_def << field_type
        end
      end
      klass = ::Struct.new(*(event_def.map { |(field,_)| field }))
      klass = parent_class.const_set(klass_sym, klass)
      type_db = klass.const_set :TYPE_DB, {}
      event_def.each { |(field,type)| type_db[field] = type }
      klass.const_set :TYPE_LIST, event_def
      klass.class_eval(&block) if block_given?
      klass
    end
  end
end
