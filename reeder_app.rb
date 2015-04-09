require 'httparty'
require 'colorize'

class Reddit
  include HTTParty
  base_uri 'http://www.reddit.com/.json?limit=10'


  def news
    puts "=====================================================================".colorize(:red)
    puts "\t\t\t\tReddit".colorize(:red)
    response = self.class.get('')
    response["data"]["children"].each do |new|
    puts "#{new["data"]["author"]}".colorize(:blue)  
    puts "#{new["data"]["title"]} "
    puts "#{new["data"]["created_utc"]}"
    puts "#{new["data"]["url"]}\n\n"
    end
  end
end

class Mashable
  include HTTParty
  base_uri 'http://www.reddit.com/.json?limit=10'


  def news
    puts "=====================================================================".colorize(:red)
    puts "\t\t\t\tMashable".colorize(:red)

    response = HTTParty.get("http://mashable.com/stories.json?limit=10")
    response["hot"].each do |new|
    puts "#{new["author"]}".colorize(:blue)  
    puts "#{new["title"]} "
    puts "#{new["post_date"]}"
    puts "#{new["link"]}\n\n"
    end
  end
end

class Digg
  include HTTParty
  base_uri 'https://digg.com/api/news/popular.json'


  def news
    puts "=====================================================================".colorize(:red)
    puts "\t\t\t\tDigg".colorize(:red)

    response = HTTParty.get("https://digg.com/api/news/popular.json")
    response["data"]["feed"].each do |new|
    puts "#{new["content"]["author"]}".colorize(:blue)  
    puts "#{new["content"]["title_alt"]}"
    puts "#{new["date_published"]}"
    puts "#{new["content"]["url"]}\n\n"
    end
  end
end



def main
  reddit_feed = Reddit.new
  reddit_feed.news
  mashable_feed = Mashable.new
  mashable_feed.news
  digg_feed = Digg.new
  digg_feed.news
end

main



