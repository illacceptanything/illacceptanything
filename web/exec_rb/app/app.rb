Kernel.define_method :jquery do |selector|
  MrubyJs.window.jQuery(selector)
end

Kernel.define_method :j do |selector|
  jquery(selector)
end

Kernel.define_method :window do
  MrubyJs.window
end

Kernel.define_method :w do
  window
end
