class LWES::TypeDB

  # create LWES::Struct-derived classes based on the contents
  # of the TypeDB object.  It is possible to place all classes
  # into a namespace by specifying the :parent option to point
  # to a class or module:
  #
  #   module MyEvents; end
  #
  #   type_db = LWES::TypeDB.new("my_events.esf")
  #   type_db.create_classes!(:parent => MyEvents)
  #   type_db.create_classes!(:sparse => true)
  #
  # Assuming you had "Event1" and "Event2" defined in your "my_events.esf"
  # file, then the classes MyEvents::Event1 and MyEvents::Event2 should
  # now be accessible.
  #
  #   :parent   - parent class or module, the default is 'Object' putting
  #               the new class in the global namespace.  May be +nil+ for
  #               creating anonymous classes.
  #   :sparse   - If +true+, this will subclass from LWES::Event instead of
  #               :Struct for better memory efficiency when dealing with
  #               events with many unused fields.  Default is +false+.
  def create_classes!(options = {})
    classes = to_hash.keys - [ :MetaEventInfo ]
    classes.sort { |a,b| a.to_s.size <=> b.to_s.size }.map! do |klass|
      opts = { :db => self, :class => klass }.merge!(options)
      opts[:sparse] ? LWES::Event.subclass(opts) : LWES::Struct.new(opts)
    end
  end

  # :stopdoc:
  # avoid GC mis-free-ing nuked objects
  def dup
    self
  end
  alias clone dup
end
