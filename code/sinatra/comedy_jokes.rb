require 'bundler/setup'
require 'sinatra'
require 'sinatra/reloader'

get '/' do
  "Hello!"
end

get '/jokes' do
  "Why couldn't the small grey bear make partner? He wasn't koala-fied."
end

get '/form' do
  erb :form
end

post '/form' do
  "You said '#{params[:message]}'"
end

get '/:name' do
  "Hey #{params[:name]}."
end

get '/jokes/:name' do
  "Hey #{params[:name]}, why couldn't the small grey bear make partner? He
          wasn't koala-fied. Get it #{params[:name]}???"
end
