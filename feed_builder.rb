#
# FINALLY - Feed Support!
# 
# Run it with "ruby feed_builder.rb" and it will recreate feed.rss.
# Subscribe via. https://raw.githubusercontent.com/mrkrstphr/illacceptanything/master/feed.rss
#
require 'digest'

feed_start = <<FEED_START
<?xml version="1.0" encoding="utf-8"?>
<rss version="2.0">
  <channel>
    <title>illacceptanything</title>
    <link>https://github.com/mrkrstphr/illacceptanything</link>
    <description> The project where literally* anything goes</description>
    <language>en-en</language>
FEED_START

feed_items = ""
Dir.glob('**/*').each do |file|
  feed_items += <<FEED_ITEM
      <item>
        <title>#{file}</title>
        <description>https://raw.githubusercontent.com/mrkrstphr/illacceptanything/master/#{file}</description>
        <link>https://raw.githubusercontent.com/mrkrstphr/illacceptanything/master/#{file}</link>
        <author>illacceptanything</author>
        <guid>#{Digest::MD5.hexdigest(file)}</guid>
        <pubDate>#{Time.now}</pubDate>
      </item>
FEED_ITEM
end

feed_end = <<FEED_END
  </channel>
</rss>
FEED_END

feed_content = feed_start + feed_items + feed_end
rss = File.open('feed.rss','w')
rss.write(feed_content)
