module LWES
  class Struct

    # creates a new Struct-based class, takes the following
    # options hash:
    #
    #   :file     - pathname to the ESF file, this is always required
    #   :class    - Ruby base class name, if the ESF file only has one
    #               event defined (besides MetaEventInfo), then specifying
    #               it is optional, otherwise it is required when multiple
    #               events are defined in the same ESF :file given above
    #   :parent   - parent class or module, the default is 'Object' putting
    #               the new class in the global namespace.
    #   :event    - event name if it differs from the Ruby base class name
    #               given (or inferred) above.  For DRY-ness, you are
    #               recommended to keep your event names and Ruby class
    #               names in sync and not need this option.
    #   :optional - Array of field names that are optional, the special
    #               "MetaEventInfo" name makes all elements defined in
    #               the MetaEventInfo section of the ESF is optional.
    #               This may also be a regular expression.
    #   :skip     - Array of field names to skip from the Event defininition
    #               entirely, these could include fields that are only
    #               implemented by the Listener.  This may also be a
    #               regular expression.
    #   :defaults - hash of default key -> value pairs to set at
    #               creation time
    #
    def self.new(options, &block)
      file = options[:file] or raise ArgumentError, "esf file missing"
      test ?r, file or raise ArgumentError, "file #{file.inspect} not readable"
      dump = TypeDB.new(file).to_hash
      klass = options[:class] || begin
        # make it easier to deal with single event files
        events = (dump.keys -  [ :MetaEventInfo ])
        events.size > 1 and
            raise RuntimeError,
                  "multiple event defs available: #{events.inspect}\n" \
                  "pick one with :class"
        events.first
      end

      name = options[:name] || klass
      parent = options[:parent] || Object
      event_def = dump[name.to_sym] or
        raise RuntimeError, "#{name.inspect} not defined in #{file}"

      # merge MetaEventInfo fields in
      meta_event_info = dump[:MetaEventInfo]
      alpha = proc { |a,b| a.first.to_s <=> b.first.to_s }
      event_def = event_def.sort(&alpha)
      if meta_event_info
        seen = event_def.map { |(field, _)| field }
        meta_event_info.sort(&alpha).each do |field_type|
          seen.include?(field_type.first) or event_def << field_type
        end
      end

      Array(options[:skip]).each do |x|
        if Regexp === x
          event_def.delete_if { |(f,_)| x =~ f.to_s }
        else
          if x.to_sym == :MetaEventInfo
            meta_event_info.nil? and
              raise RuntimeError, "MetaEventInfo not defined in #{file}"
            meta_event_info.each do |(field,_)|
              event_def.delete_if { |(f,_)| field == f }
            end
          else
            event_def.delete_if { |(f,_)| f == x.to_sym }
          end
        end
      end

      tmp = ::Struct.new(*(event_def.map { |(field,_)| field }))
      tmp = parent.const_set(klass, tmp)
      type_db = tmp.const_set :TYPE_DB, {}
      event_def.each { |(field,type)| type_db[field] = type }
      tmp.const_set :TYPE_LIST, event_def

      optional = tmp.const_set :OPTIONAL, {}
      Array(options[:optional]).each do |x|
        if Regexp === x
          event_def.each { |(f,_)| optional[f] = true if x =~ f.to_s }
        else
          if x.to_sym == :MetaEventInfo
            meta_event_info.nil? and
              raise RuntimeError, "MetaEventInfo not defined in #{file}"
            meta_event_info.each{ |(field,_)| optional[field] = true }
          else
            optional[x.to_sym] = true
          end
        end
      end
      defaults = options[:defaults] || {}
      defaults = tmp.const_set :DEFAULTS, defaults.dup

      # define a parent-level method, so instead of Event.new
      # you could do
      if false
        (parent == Object ?
          Kernel : parent.instance_eval { class << self; self; end }
        ).__send__(:define_method, klass) do |*args|
          if Hash === (init = args.first)
            rv = tmp.new()
            defaults.merge(init).each_pair { |k,v| rv[k] = v }
            rv
          else
            rv = tmp.new(*args)
            defaults.each_pair { |k,v| rv[k] ||= v }
            rv
          end
        end
      else
        tmp.instance_eval { class << self; self; end }.instance_eval do
          alias_method :_new, :new
          define_method(:new) do |*args|
            if Hash === (init = args.first)
              rv = tmp._new()
              defaults.merge(init).each_pair { |k,v| rv[k] = v }
              rv
            else
              rv = tmp._new(*args)
              defaults.each_pair { |k,v| rv[k] ||= v }
              rv
            end
          end
        end
      end

      tmp.class_eval(&block) if block_given?
      tmp
    end
  end
end
