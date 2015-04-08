lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)

require "illacceptanything/version"

Gem::Specification.new do |spec|
  spec.name        = "illacceptanything"
  spec.version     = Illacceptanything::VERSION
  spec.authors     = ["Kristopher Wilson"]
  spec.email       = ["kristopherwilson@gmail.com>"]
  spec.homepage    = "https://github.com/mrkrstphr/illacceptanything"
  spec.summary     = "I'll accept anything"
  spec.description = "The project where literally* anything goes"
  spec.license     = "MIT"

  spec.files         = `git ls-files -z`.split("\x0").reject { |f| f.match(%r{^(test|spec|features)/}) }
  spec.require_paths = ["lib"]

  spec.add_development_dependency "bundler", "~> 1.9"
  spec.add_development_dependency "rake", "~> 10.0"
end
