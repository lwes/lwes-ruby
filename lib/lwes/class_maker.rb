module LWES::ClassMaker
  def type_db(options)
    options[:db] || begin
      file = options[:file] or
        raise ArgumentError, "TypeDB :db or ESF :file missing"
      File.readable?(file) or
        raise ArgumentError, "file #{file.inspect} not readable"
      LWES::TypeDB.new(file)
    end
  end

  def class_for(options, dump)
    klass = options[:class] || begin
      # make it easier to deal with single event files
      events = (dump.keys -  [ :MetaEventInfo ])
      events.size > 1 and
          raise RuntimeError,
                "multiple event defs available: #{events.inspect}\n" \
                "pick one with :class"
      events.first
    end
    name = options[:name] || klass.to_s
    event_def = dump[name.to_sym] or
      raise RuntimeError, "#{name.inspect} not defined in #{file}"
    [ klass, name, event_def ]
  end

  def set_constants(tmp, db, klass, name, options)
    tmp.const_set :TYPE_DB, db
    tmp.const_set :NAME, name.to_s.dup.freeze
    defaults = options[:defaults] || {}
    tmp.const_set :DEFAULTS, defaults.dup
    parent = options.include?(:parent) ? options[:parent] : Object
    parent or return
    components = klass.to_s.split(/::/)
    components.each_with_index do |component, i|
      if i == (components.size - 1)
        tmp = parent.const_set(component, tmp)
      else
        parent = begin
          parent.const_get(component)
        rescue NameError
          parent.const_set component, Module.new
        end
      end
    end
  end
end
