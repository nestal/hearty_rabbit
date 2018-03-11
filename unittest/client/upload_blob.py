import requests
import unittest
from PIL import Image
import io

class NormalTestCase(unittest.TestCase):
	def setUp(self):
		self.session = requests.Session()
		self.session.verify = "../../etc/hearty_rabbit/certificate.pem"

		login_response = self.session.post(
			"https://localhost:4433/login",
			data="username=sumsum&password=bearbear",
			headers={"Content-type": "application/x-www-form-urlencoded"}
		)
		self.assertEqual(login_response.status_code, 200)
		self.assertNotEqual(self.session.cookies.get("id"), "")

	def tearDown(self):
		self.session.close()

	def test_fetch_home_page(self):
		r1 = self.session.get("https://localhost:4433")
		self.assertEqual(r1.status_code, 200)

	def test_upload(self):
		with open("../tests/image/black.jpg", 'rb') as black:
			in_img = black.read()

			r1 = self.session.put("https://localhost:4433/upload/test.jpg", data=in_img)
			self.assertEqual(r1.status_code, 201)
			self.assertNotEqual(r1.headers["Location"], "")

			# read back the upload image
			r2 = self.session.get("https://localhost:4433" + r1.headers["Location"])
			self.assertEqual(r2.status_code, 200);
			jpeg = Image.open(io.BytesIO(r2.content))



if __name__ == '__main__':
	unittest.main()
