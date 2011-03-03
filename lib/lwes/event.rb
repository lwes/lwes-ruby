# this class is incomplete, but will eventually be subclassable
# like Struct in Ruby
class LWES::Event
  SYM2ATTR = Hash.new { |h,k| h[k] = k.to_s.freeze } # :nodoc:

  def self.subclass(name, type_db)
    klass = Class.new(self)
    klass.const_set :TYPE_DB, type_db
    klass.const_set :NAME, name.to_s.dup.freeze
    dump = type_db.to_hash
    meta = dump[:MetaEventInfo] || []
    methods = meta + dump[name.to_sym]
    methods = methods.inject("") do |str, (k,_)|
      str << "def #{k}; self[:#{k}]; end\n"
      str << "def #{k}= val; self[:#{k}] = val; end\n"
    end
    klass.class_eval methods
    klass
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
    src.to_hash.each do |k,v|
      begin
        self[k] = v
      rescue ArgumentError
        raise if k != :name
      end
    end
    self
  end

  alias_method :initialize_copy, :merge!

  def merge src
    dup.merge! src
  end
end
