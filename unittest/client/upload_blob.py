import http.client
import ssl
import unittest

class NormalTestCase(unittest.TestCase):
    def test_normal_http(self):
        conn = http.client.HTTPSConnection("localhost:4433", context=ssl._create_unverified_context())
        conn.request("GET", "/")
        r1 = conn.getresponse()
        self.assertEqual(r1.status, 200)
        conn.close()

unittest.main()
