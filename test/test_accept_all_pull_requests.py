import json
import requests
import unittest

URL = 'https://api.github.com/repos/mrkrstphr/illacceptanything/pulls?state=open'

class AcceptAllPullRequestsTestCase(unittest.TestCase):
    def testAcceptAllPullRequests(self):
        content = json.loads(requests.get(URL).text)
        self.assertEqual(content, [])
