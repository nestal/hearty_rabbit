import requests
import unittest

class NormalTestCase(unittest.TestCase):
	def test_fetch_home_page(self):
		r = requests.get("https://localhost:4433", verify=False)

if __name__ == '__main__':
	unittest.main()
