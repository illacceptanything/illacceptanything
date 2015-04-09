require 'test/unit'
require 'json'
require 'open-uri'

class TestEverything < Test::Unit::TestCase

  def windspeed_in_angola
    weather_in_angola = open("http://api.openweathermap.org/data/2.5/weather?lat=18.8&lon=13.3&mode=json").read
    parsed_json = JSON.parse(weather_in_angola)
    windspeed = parsed_json['wind']['speed']
    assert(windspeed < 5.223, 'Wind is unacceptable, program may not continue')
  end

  def sun_not_yet_exploded
    assert(Time.new.year > 100000000, 'Sun may have exploded, please contact your system administrator')
  end

  def is_program_unlucky
    assert(Random.rand(99999999999999999) != 8, 'Program is unlucky, suggest not running')
  end

end
