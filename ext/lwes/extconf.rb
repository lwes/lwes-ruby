require 'net/http'
require 'mkmf'
require 'fileutils'
dir_config('lwes')

pwd = File.expand_path(File.dirname(__FILE__))
v = '0.22.3'
dir = "lwes-#{v}"
diff = "#{pwd}/#{dir}.diff"
inst = "#{pwd}/.inst"
tgz = "#{dir}.tar.gz"
url = "http://sourceforge.net/projects/lwes/files/lwes-c/#{v}/#{tgz}/download"

# from Net::HTTP example
def fetch(uri_str, limit = 10)
  raise ArgumentError, 'HTTP redirect too deep' if limit == 0

  response = Net::HTTP.get_response(URI.parse(uri_str))
  case response
  when Net::HTTPSuccess     then response
  when Net::HTTPRedirection then fetch(response['location'], limit - 1)
  else
    response.error!
  end
end

unless have_library('lwes') && have_header('lwes.h')
  warn "LWES library not found, building locally"
  Dir.chdir(pwd) do
    unless test ?r, tgz
      response = fetch(url)
      File.open("#{tgz}.#{$$}.#{rand}.tmp", "wb") do |fp|
        fp.write(response.body)
        File.rename(fp.path, tgz)
      end
    end
    unless test ?r, "#{inst}/.ok"
      FileUtils.rm_rf(dir)
      system('tar', 'zxf', tgz) or abort "tar failed with #{$?}"
      Dir.chdir(dir) do
        system("patch", "-p1", "-i", diff) or abort "patch failed: #{$?}"
        args = %w(--disable-shared
                  --disable-hardcore
                  --with-pic
                  --disable-dependency-tracking)
        system("./configure", "--prefix=#{inst}", *args) or
          abort "configure failed with #{$?}"
        system("make") or abort "make failed with #{$?}"
        system("make", "install") or abort "make install failed with #{$?}"
      end
      FileUtils.rm_rf(dir)
      File.open("#{inst}/.ok", "wb") { }
    end
    $CFLAGS = "-I#{inst}/include/lwes-0 #{$CFLAGS}"
    $LIBS += " #{inst}/lib/liblwes.a"
    have_header('lwes.h') or
      abort "installation failed"
  end
end

create_makefile('lwes_ext')
