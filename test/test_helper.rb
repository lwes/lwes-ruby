require 'pp'
require 'tempfile'
require 'test/unit'
require 'lwes'
GC.stress = true if ENV["GC_STRESS"].to_i != 0

unless defined?(LISTENER_DEFAULTS)
  BEFORE_DELAY = ENV['BEFORE_DELAY'] ? ENV['BEFORE_DELAY'].to_f : 0.5
  AFTER_DELAY = ENV['AFTER_DELAY'] ? ENV['AFTER_DELAY'].to_f : 0.5
  LISTENER_DEFAULTS = {
    :address => ENV["LWES_TEST_ADDRESS"] || "127.0.0.1",
    :iface => ENV["LWES_TEST_IFACE"] || "0.0.0.0",
    :port => ENV["LWES_TEST_PORT"] ? ENV["LWES_TEST_PORT"].to_i : 12345,
    :ttl => 60, # nil for no ttl)
  }
end

private_bin = "ext/lwes_ext/.inst/bin"
if File.directory? private_bin
  ENV['PATH'] = "#{private_bin}:#{ENV['PATH']}"
end

def lwes_listener(&block)
  cmd = "lwes-event-printing-listener" \
        " -m #{@options[:address]}" \
        " -i #{@options[:iface]}" \
        " -p #{@options[:port]}"
  out = Tempfile.new("out")
  err = Tempfile.new("err")
  $stdout.flush
  $stderr.flush
  pid = fork do
    $stdout.reopen(out.path)
    $stderr.reopen(err.path)
    exec cmd
  end
  begin
    # since everything executes asynchronously and our messaging,
    # we need to ensure our listener is ready, then ensure our
    # listener has printed something...
    # XXX racy
    sleep BEFORE_DELAY
    yield
    sleep AFTER_DELAY
  ensure
    Process.kill(:TERM, pid)
    Process.waitpid2(pid)
    assert_equal 0, err.size
  end
  out
end
