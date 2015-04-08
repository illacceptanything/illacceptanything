require 'sinatra'
require 'sinatra/reloader'

get '/' do
  return "Hello!"
end

get '/jokes' do
  return "Why couldn't the small grey bear make partner? He wasn't koala-fied."
end

get '/:name' do
  return "Hey #{params[:name]}."
end

get '/jokes/:name' do
  return "Hey #{params[:name]}, why couldn't the small grey bear make partner? He
          wasn't koala-fied. Get it #{params[:name]}???"
end
