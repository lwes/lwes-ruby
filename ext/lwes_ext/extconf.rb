require 'net/http'
require 'mkmf'
require 'fileutils'
dir_config('lwes')
have_func('rb_thread_blocking_region', 'ruby.h')

pwd = File.expand_path(File.dirname(__FILE__))
v = '0.23.1'
dir = "lwes-#{v}"
diff = "#{pwd}/#{dir}.diff"
inst = "#{pwd}/.inst"
tgz = "#{dir}.tar.gz"
url = "http://sourceforge.net/projects/lwes/files/lwes-c/#{v}/#{tgz}/download"

def sub_cd(dir, &b)
  oldpwd = ENV["PWD"]
  begin
    ENV["PWD"] = dir
    Dir.chdir(dir, &b)
  ensure
    if oldpwd
      ENV["PWD"] = oldpwd
    else
      ENV.delete("PWD")
    end
  end
end

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

unless have_header('lwes.h') && have_library('lwes')
  warn "LWES library not found, building locally"
  sub_cd(pwd) do
    unless File.readable?(tgz)
      response = fetch(url)
      File.open("#{tgz}.#{$$}.#{rand}.tmp", "wb") do |fp|
        fp.write(response.body)
        File.rename(fp.path, tgz)
      end
    end
    unless File.readable?("#{inst}/.ok")
      FileUtils.rm_rf(dir)
      system('tar', 'zxf', tgz) or abort "tar failed with #{$?}"
      sub_cd(dir) do
        if File.exist?(diff)
          system("patch", "-p1", "-i", diff) or abort "patch failed: #{$?}"
        end
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
