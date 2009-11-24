module LWES
  class Struct

    # creates a new Struct-based class, takes the following
    # options hash:
    #
    #   :db       - pre-created LWES::TypeDB object
    #               this is required unless :file is given
    #   :file     - pathname to the ESF file,
    #               this is required unless :db is given
    #   :class    - Ruby base class name, if the ESF file only has one
    #               event defined (besides MetaEventInfo), then specifying
    #               it is optional, otherwise it is required when multiple
    #               events are defined in the same ESF :file given above
    #   :parent   - parent class or module, the default is 'Object' putting
    #               the new class in the global namespace.
    #   :name     - event name if it differs from the Ruby base class name
    #               given (or inferred) above.  For DRY-ness, you are
    #               recommended to keep your event names and Ruby class
    #               names in sync and not need this option.
    #   :skip     - Array of field names to skip from the Event defininition
    #               entirely, these could include fields that are only
    #               implemented by the Listener.  This may also be a
    #               regular expression.
    #   :defaults - hash of default key -> value pairs to set at
    #               creation time
    #
    def self.new(options, &block)
      db = options[:db]
      db ||= begin
        file = options[:file] or
          raise ArgumentError, "TypeDB :db or ESF :file missing"
        test ?r, file or
          raise ArgumentError, "file #{file.inspect} not readable"
        TypeDB.new(file)
      end
      dump = db.to_hash
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
      tmp.const_set :TYPE_DB, db
      tmp.const_set :NAME, name.to_s
      ed = tmp.const_set :EVENT_DEF, {}
      event_def.each { |(field,type)| ed[field] = type }
      type_list = event_def.map { |(field,type)| [ field, field.to_s, type ] }
      tmp.const_set :TYPE_LIST, type_list

      defaults = options[:defaults] || {}
      defaults = tmp.const_set :DEFAULTS, defaults.dup
      tmp.class_eval(&block) if block_given?

      # define a parent-level method, eval is faster than define_method
      eval <<EOS
class ::#{tmp.name}
  class << self
    alias _new new
    undef_method :new
    def new(*args)
      if Hash === (init = args.first)
        rv = _new()
        DEFAULTS.merge(init).each_pair { |k,v| rv[k] = v }
        rv
      else
        rv = _new(*args)
        DEFAULTS.each_pair { |k,v| rv[k] ||= v }
        rv
      end
    end
  end
end
EOS
    tmp
    end
  end
end
