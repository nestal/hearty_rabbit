import requests
import unittest
from PIL import Image
import io

class NormalTestCase(unittest.TestCase):
	def setUp(self):
		# set up a session with valid credential
		self.session = requests.Session()
		self.session.verify = "../../etc/hearty_rabbit/certificate.pem"

		login_response = self.session.post(
			"https://localhost:4433/login",
			data="username=sumsum&password=bearbear",
			headers={"Content-type": "application/x-www-form-urlencoded"}
		)
		self.assertEqual(login_response.status_code, 200)
		self.assertNotEqual(self.session.cookies.get("id"), "")

		# set up a session without valid credential
		self.session_no_cred = requests.Session()
		self.session_no_cred.verify = "../../etc/hearty_rabbit/certificate.pem"

	def tearDown(self):
		self.session.close()

	def test_fetch_home_page(self):
		r1 = self.session.get("https://localhost:4433")
		self.assertEqual(r1.status_code, 200)

	def test_upload(self):
		with open("../tests/image/black.jpg", 'rb') as black:

			# read source image
			in_img = black.read()
			source_jpeg = Image.open(io.BytesIO(in_img))

			# upload to server
			r1 = self.session.put("https://localhost:4433/upload/test.jpg", data=in_img)
			self.assertEqual(r1.status_code, 201)
			self.assertNotEqual(r1.headers["Location"], "")

			# read back the upload image
			r2 = self.session.get("https://localhost:4433" + r1.headers["Location"])
			self.assertEqual(r2.status_code, 200)
			jpeg = Image.open(io.BytesIO(r2.content))

			# the size of the images should be the same
			self.assertEqual(jpeg.width, source_jpeg.width)
			self.assertEqual(jpeg.height, source_jpeg.height)

			# cannot get the same image without credential
			r3 = self.session_no_cred.get("https://localhost:4433" + r1.headers["Location"])
			self.assertEqual(r3.status_code, 403)

	def test_not_found(self):
		# resource not exist
		r1 = self.session.get("https://localhost:4433/index.html")
		self.assertEqual(r1.status_code, 404)

		# invalid blob
		r2 = self.session.get("https://localhost:4433/blob/sumsum/0000000000000000000000000000000000000000")
		self.assertEqual(r2.status_code, 404)

		# other user's blob
		r3 = self.session.get("https://localhost:4433/blob/nestal/0100000000000000000000000000000000000003")
		self.assertEqual(r3.status_code, 403)

if __name__ == '__main__':
	unittest.main()
