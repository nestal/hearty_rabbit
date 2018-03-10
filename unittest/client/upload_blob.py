import http.client
import http.cookies
import ssl
import unittest

class NormalTestCase(unittest.TestCase):
	def test_fetch_home_page(self):
		conn = http.client.HTTPSConnection("localhost:4433", context=ssl._create_unverified_context())
		conn.request("GET", "/")
		r1 = conn.getresponse()
		self.assertEqual(r1.status, 200)

		# check response content
		data = r1.read()
		self.assertGreater(len(data), 0)

		conn.close()

	def test_login(self):
		headers = {"Content-type": "application/x-www-form-urlencoded"}

		conn = http.client.HTTPSConnection("localhost:4433", context=ssl._create_unverified_context())
		conn.request("POST", "/login", "username=nestal&password=hello", headers)
		r1 = conn.getresponse()
		self.assertEqual(r1.status, 303)
		self.assertEqual(r1.getheader("Location"), "/view/nestal/")

		cookie1 = cookies.SimpleCookie()
		cookie1.load(r1.getheader("Set-Cookie"))

		r1.read()
		conn.close()

if __name__ == '__main__':
	unittest.main()
