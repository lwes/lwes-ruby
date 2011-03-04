Gem::Specification.new do |s|
  s.name = %q{lwes}
  s.version = "0.8.0pre1"
  s.date = Time.now
  s.authors = ["Erik S. Chang", "Frank Maritato"]
  s.email = %q{lwes-devel@lists.sourceforge.net}
  s.summary = %q{Ruby bindings for the Light Weight Event System}
  s.homepage = %q{http://lwes.rubyforge.org/}
  s.extensions = %w(ext/lwes_ext/extconf.rb)
  s.description = %q{
The LWES Light-Weight Event System is a framework for allowing the exchange of
information from many machines to many machines in a controlled, platform
neutral, language neutral way.  The exchange of information is done in a
connectless fashion using multicast or unicast UDP, and using self describing
data so that any platform or language can translate it to it's local dialect.
}.strip
  s.files = `git ls-files`.split(/\n/) +
            %w(ext/lwes_ext/lwes-0.23.1.tar.gz)
  s.rubyforge_project = 'lwes'
  s.test_files = s.files.grep(%r{\Atest/unit/test_})
  s.add_development_dependency(%q<rake-compiler>, [">= 0.7.6"])
end
