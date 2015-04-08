require 'webruby'

# This file sets up the build environment for a webruby project.
Webruby::App.setup do |conf|
  # Entrypoint file name
  conf.entrypoint = 'app/app.rb'

  # By default, the build output directory is "build/"
  conf.build_dir = 'build'

  # Use 'release' for O2 mode build, and everything else for O0 mode
  conf.compile_mode = 'debug'

  # Loading mode, see lib/webruby/utility.rb for details
  conf.loading_mode = 2

  # 2 Ruby source processors are available right now:
  #
  # :mrubymix - The old one supporting static require
  # :gen_require - The new one supporting require
  conf.source_processor = :mrubymix

  # By default the final output file name is "webruby.js"
  conf.output_name = 'webruby.js'

  # You can append a JS file at the end of the final output file
  # For example, a runner file like following can be used to run
  # Ruby code automatically:
  #
  # (function () {
  #    var w = new WEBRUBY();
  #    w.run();
  # }) ();
  #
  # NOTE: We used to support a js_bin target which will compile
  # a `main.c` file to run the code, but now we favor this method
  # instead of the old one.
  # conf.append_file = 'runner.js'

  # The syntax for adding gems here are kept the same as mruby.
  # Below are a few examples:

  # mruby-eval gem, all parsing code will be packed into the final JS!
  conf.gem :core => "mruby-eval"

  # JavaScript calling interface
  conf.gem :git => 'git://github.com/xxuejie/mruby-js.git', :branch => 'master'
  conf.gem :git => 'git://github.com/xxuejie/webruby-multiline-parse.git'

  # OpenGL ES 2.0 binding
  # conf.gem :git => 'git://github.com/xxuejie/mruby-gles.git', :branch => 'master'

  # Normally we wouldn't use this example gem, I just put it here to show how to
  # add a gem on the local file system, you can either use absolute path or relative
  # path from mruby root, which is modules/webruby.
  # conf.gem "#{root}/examples/mrbgems/c_and_ruby_extension_example"
end

Rake::Task[:default].enhance do
  cp "build/webruby.js", "js/webruby.js", verbose: true
end
