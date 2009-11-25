module LWES
  class TypeDB

    # create LWES::Struct-derived classes based on the contents
    # of the TypeDB object.  It is possible to place all classes
    # into a namespace by specifying the :parent option to point
    # to a class or module:
    #
    #   module MyEvents; end
    #
    #   type_db = LWES::TypeDB.new("my_events.esf")
    #   type_db.create_classes!(:parent => MyEvents)
    #
    # Assuming you had "Event1" and "Event2" defined in your "my_events.esf"
    # file, then the classes MyEvents::Event1 and MyEvents::Event2 should
    # now be accessible.
    def create_classes!(options = {})
      classes = to_hash.keys - [ :MetaEventInfo ]
      classes.sort { |a,b| a.to_s.size <=> b.to_s.size }.map do |klass|
        LWES::Struct.new({ :db => self, :class => klass }.merge(options))
      end
    end

  end
end
