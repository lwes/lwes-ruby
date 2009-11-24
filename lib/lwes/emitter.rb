module LWES
  class Emitter

    # creates a new Emitter object which may be used for the lifetime
    # of the process:
    #
    #   LWES::Emitter.new(:address => '224.1.1.11',
    #                     :iface => '0.0.0.0',
    #                     :port => 12345,
    #                     :heartbeat => false, # Integer for frequency
    #                     :ttl => 60, # nil for no ttl)
    #
    def initialize(options = {}, &block)
      options[:iface] ||= '0.0.0.0'
      _create(options)
      block_given?
    end
  end
end
