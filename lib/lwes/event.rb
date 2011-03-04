# Used for mapping LWES events in in an ESF file to a Ruby object.
# LWES::Event-derived classes are more memory efficient if your event
# definitions have many unused fields.
#
# LWES::TypeDB.create_classes! with +:sparse+ set to +true+
class LWES::Event
  SYM2ATTR = Hash.new { |h,k| h[k] = k.to_s.freeze } # :nodoc:

  # used to cache classes for LWES::Event.parse
  CLASSES = {} # :nodoc:
  extend LWES::ClassMaker

  def self.subclass(options, &block)
    db = type_db(options)
    dump = db.to_hash
    klass, name, event_def = class_for(options, dump)
    tmp = Class.new(self)
    set_constants(tmp, db, klass, name, options)
    tmp.class_eval(&block) if block_given?

    meta = dump[:MetaEventInfo] || []
    methods = meta + event_def
    methods = methods.inject("") do |str, (k,_)|
      str << "def #{k}; self[:#{k}]; end\n"
      str << "def #{k}= val; self[:#{k}] = val; end\n"
    end
    methods << "def initialize(src = nil); merge!(DEFAULTS); super; end\n"
    tmp.class_eval methods
    CLASSES[name] = tmp
  end

  def inspect
    klass = self.class
    if LWES::Event == klass
      "#<#{klass}:#{to_hash.inspect}>"
    else
      "#<#{klass}(event:#{klass.const_get(:NAME)}):#{to_hash.inspect}>"
    end
  end

  def merge! src
    src.to_hash.each { |k,v| self[k] = v }
    self
  end

  alias_method :initialize_copy, :merge!

  def merge src
    dup.merge! src
  end

private

  def initialize(src = nil)
    src and merge! src
  end
end
