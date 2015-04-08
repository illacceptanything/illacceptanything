require "open-uri"
require "test/unit"

class AcceptAllPullRequestsTest < Test::Unit::TestCase
  URL='https://api.github.com/repos/mrkrstphr/illacceptanything/pulls?state=open'
 
  def test_accept_all_pull_requests
    content=open(URL).read
    assert_equal content, '[]'
  end
 
end