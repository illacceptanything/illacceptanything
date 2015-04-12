#!/usr/bin/env ruby

require 'sinatra'

get '/' do
  "<b>I'll accept anything.</b>"
end

get '/*' do
  "<a href=\"/\">Link</a>"
end
