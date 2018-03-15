import requests
import unittest
from PIL import Image
import io
import json

class NormalTestCase(unittest.TestCase):
	def setUp(self):
		# set up a session with valid credential
		self.user1 = requests.Session()
		self.user1.verify = "../../etc/hearty_rabbit/certificate.pem"

		login_response = self.user1.post(
			"https://localhost:4433/login",
			data="username=sumsum&password=bearbear",
			headers={"Content-type": "application/x-www-form-urlencoded"}
		)
		self.assertEqual(login_response.status_code, 200)
		self.assertNotEqual(self.user1.cookies.get("id"), "")

		self.user2 = requests.Session()
		self.user2.verify = "../../etc/hearty_rabbit/certificate.pem"
		self.user2.post(
			"https://localhost:4433/login",
			data="username=siuyung&password=rabbit",
			headers={"Content-type": "application/x-www-form-urlencoded"}
		)

		# set up a session without valid credential
		self.session_no_cred = requests.Session()
		self.session_no_cred.verify = "../../etc/hearty_rabbit/certificate.pem"

	def tearDown(self):
		self.user1.close()
		self.user2.close()
		self.session_no_cred.close()

	def test_fetch_home_page(self):
		r1 = self.user1.get("https://localhost:4433")
		self.assertEqual(r1.status_code, 200)

	def test_upload_jpeg(self):
		with open("../tests/image/black.jpg", 'rb') as black:

			# read source image
			in_img = black.read()
			source_jpeg = Image.open(io.BytesIO(in_img))

			# upload to server
			r1 = self.user1.put("https://localhost:4433/upload/sumsum/test.jpg", data=in_img)
			self.assertEqual(r1.status_code, 201)
			self.assertNotEqual(r1.headers["Location"], "")

			# read back the upload image
			r2 = self.user1.get("https://localhost:4433" + r1.headers["Location"])
			self.assertEqual(r2.status_code, 200)
			self.assertEqual(r2.headers["Content-type"], "image/jpeg")
			jpeg = Image.open(io.BytesIO(r2.content))

			# the size of the images should be the same
			self.assertEqual(jpeg.width, source_jpeg.width)
			self.assertEqual(jpeg.height, source_jpeg.height)

			# cannot get the same image without credential
			r3 = self.session_no_cred.get("https://localhost:4433" + r1.headers["Location"])
			self.assertEqual(r3.status_code, 403)

	def test_upload_png(self):
		with open("../tests/image/black_20x20.png", 'rb') as black:
			# upload to server
			r1 = self.user1.put("https://localhost:4433/upload/sumsum/black.png", data=black)
			self.assertEqual(r1.status_code, 201)
			self.assertNotEqual(r1.headers["Location"], "")

	def test_not_found(self):
		# resource not exist
		r1 = self.user1.get("https://localhost:4433/index.html")
		self.assertEqual(r1.status_code, 404)

		# Is all-zero blob ID really invalid? <- Yes, it's valid.
		# Requesting with a valid object ID on an object that doesn't exists will return "Not Found"
		r2 = self.user1.get("https://localhost:4433/blob/sumsum/0000000000000000000000000000000000000000")
		self.assertEqual(r2.status_code, 404)

		# other user's blob: no matter the target exists or not will give you "forbidden"
		r3 = self.user1.get("https://localhost:4433/blob/nestal/0100000000000000000000000000000000000003")
		self.assertEqual(r3.status_code, 403)

		# 10-digit blob ID is really invalid: Bad Request
		r4 = self.user1.get("https://localhost:4433/blob/sumsum/FF0000000000000000FF")
		self.assertEqual(r4.status_code, 400)

		# invalid blob ID with funny characters
		r5 = self.user1.get("https://localhost:4433/blob/nestal/0L00000000000000000PP0000000000000000003")
		self.assertEqual(r5.status_code, 400)

	def test_view_collection(self):
		# resource not exist
		r1 = self.user1.get("https://localhost:4433/coll/sumsum/")
		self.assertEqual(r1.status_code, 200)
		self.assertEqual(r1.headers["Content-type"], "application/json")
		self.assertGreater(len(r1.json()["elements"]), 0)
		self.assertEqual(r1.json()["collection"], "")
		self.assertEqual(r1.json()["username"], "sumsum")

	def test_upload_to_other_users_collection(self):
		with open("../tests/image/up_f_rot90.jpg", 'rb') as up:

			# forbidden
			r1 = self.user1.put("https://localhost:4433/upload/yungyung/abc/up.jpg", data=up)
			self.assertEqual(r1.status_code, 403)

	def test_upload_jpeg_to_other_collection(self):
		with open("../tests/image/up_f_rot90.jpg", 'rb') as up:

			# upload to server
			r1 = self.user1.put("https://localhost:4433/upload/sumsum/some/collection/up.jpg", data=up)
			self.assertEqual(r1.status_code, 201)
			blob_id = r1.headers["Location"][-40:]

		# should find it in the new collection
		r2 = self.user1.get("https://localhost:4433/coll/sumsum/some/collection/")
		self.assertEqual(r2.status_code, 200)
		self.assertEqual(r2.json()["elements"][blob_id]["filename"], "up.jpg")

		blob_url = "https://localhost:4433" + r1.headers["Location"]

		# delete it afterwards
		r3 = self.user1.delete(blob_url)
		self.assertEqual(r3.status_code, 204)

		# not found in collection
		r4 = self.user1.get("https://localhost:4433/coll/sumsum/some/collection/")
		self.assertEqual(r4.status_code, 200)
		self.assertFalse(blob_id in r4.json()["elements"])

	def test_set_permission(self):
		with open("../tests/image/up_f_upright.jpg", 'rb') as up:

			# upload to server
			r1 = self.user1.put("https://localhost:4433/upload/sumsum/some/collection/up.jpg", data=up)
			self.assertEqual(r1.status_code, 201)
			blob_id = r1.headers["Location"][-40:]

			# owner get successfully
			r2 = self.user1.get("https://localhost:4433" + r1.headers["Location"])
			self.assertEqual(r2.status_code, 200)

			# owner set permission to public
			r3 = self.user1.post(
				"https://localhost:4433" + r1.headers["Location"],
				data="perm=public",
				headers={"Content-type": "application/x-www-form-urlencoded"}
			)
			self.assertEqual(r3.status_code, 204)

			# other user can get the image
			r4 = self.user2.get("https://localhost:4433" + r1.headers["Location"])
			self.assertEqual(r4.status_code, 200)

			# owner set permission to shared
			r5 = self.user1.post(
				"https://localhost:4433" + r1.headers["Location"],
				data="perm=shared",
				headers={"Content-type": "application/x-www-form-urlencoded"}
			)
			self.assertEqual(r5.status_code, 204)

			# other user can get the image
			r6 = self.user2.get("https://localhost:4433" + r1.headers["Location"])
			self.assertEqual(r6.status_code, 200)

if __name__ == '__main__':
	unittest.main()
