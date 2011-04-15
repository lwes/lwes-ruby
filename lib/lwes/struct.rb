# LWES Events in an ESF file can be automatically turned into
# Ruby ::Struct-based objects and emitted directly through LWES::Emitter.
#
# LWES::Struct is may be more memory-efficient and faster if your application
# uses all or most of the fields in the event definition.  LWES::Event should
# be used in cases where many event fields are unused in a definition.
class LWES::Struct
  extend LWES::ClassMaker

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
  #               the new class in the global namespace.  May be +nil+ for
  #               creating anonymous classes
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
    db = type_db(options)
    dump = db.to_hash
    klass, name, event_def = class_for(options, dump)

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
    set_constants(tmp, db, klass, name, options)
    ed = tmp.const_set :EVENT_DEF, {}
    event_def.each { |(field,type)| ed[field] = type }

    # freeze since emitter.c can segfault if this ever changes
    type_list = event_def.map do |(field,type)|
      [ field, field.to_s.freeze, type ].freeze
    end.freeze
    tmp.const_set :TYPE_LIST, type_list

    aref_map = tmp.const_set :AREF_MAP, {}
    type_list.each_with_index do |(field_sym,field_str,_),idx|
      aref_map[field_sym] = aref_map[field_str] = idx
    end

    tmp.const_set :HAVE_ENCODING,
                  type_list.include?([ :enc, 'enc', LWES::INT_16 ])

    tmp.class_eval(&block) if block_given?

    # define a parent-level method, eval is faster than define_method
    tmp.class_eval <<EOS
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
EOS

  # avoid linear scans for large structs, not sure if 50 is a good enough
  # threshold but it won't help for anything <= 10 since Ruby (or at least
  # MRI) already optimizes those cases
  if event_def.size > 50
    tmp.class_eval <<EOS
alias __aref []
alias __aset []=
def [](key)
  __aref(key.kind_of?(Fixnum) ? key : AREF_MAP[key])
end

def []=(key, value)
  __aset(key.kind_of?(Fixnum) ? key : AREF_MAP[key], value)
end
EOS
    fast_methods = []
    event_def.each_with_index do |(fld,_), idx|
      next if idx <= 9
      if idx != aref_map[fld]
        raise LoadError, "event_def corrupt: #{event_def}"
      end
      fast_methods << "undef_method :#{fld}, :#{fld}=\n"
      fast_methods << "\ndef #{fld}; __aref #{idx}; end\n"
      fast_methods << "\ndef #{fld}=(val); __aset #{idx}, val ; end\n"
    end

    tmp.class_eval fast_methods.join("\n")
  end

  tmp
  end
end
