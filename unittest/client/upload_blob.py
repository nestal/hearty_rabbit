import requests
import unittest
from PIL import Image
from io import BytesIO
import numpy
import json

class NormalTestCase(unittest.TestCase):
	# without the "test" prefix in the method, it is not treated as a test routine
	@staticmethod
	def random_image(width, height, format="jpeg"):
		imarray = numpy.random.rand(height, width, 3) * 255
		img = Image.fromarray(imarray.astype("uint8")).convert("RGB")
		temp = BytesIO()
		img.save(temp, format=format)
		return temp.getvalue()

	def get_collection(self, session, owner, coll):
		response = session.get("https://localhost:4433/coll/" + owner + "/" + coll + "/")
		self.assertEqual(response.status_code, 200)
		self.assertEqual(response.headers["Content-type"], "application/json")
		return response.json()

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
		self.anon = requests.Session()
		self.anon.verify = "../../etc/hearty_rabbit/certificate.pem"

	def tearDown(self):
		self.user1.close()
		self.user2.close()
		self.anon.close()

	def test_fetch_home_page(self):
		r1 = self.user1.get("https://localhost:4433")
		self.assertEqual(r1.status_code, 200)

	def test_upload_jpeg(self):
		# upload to server
		r1 = self.user1.put("https://localhost:4433/upload/sumsum/test.jpg", data=self.random_image(72, 36))
		self.assertEqual(r1.status_code, 201)
		self.assertNotEqual(r1.headers["Location"], "")

		# read back the upload image
		r2 = self.user1.get("https://localhost:4433" + r1.headers["Location"])
		self.assertEqual(r2.status_code, 200)
		self.assertEqual(r2.headers["Content-type"], "image/jpeg")
		jpeg = Image.open(BytesIO(r2.content))

		# the size of the images should be the same
		self.assertEqual(jpeg.format, "JPEG")
		self.assertEqual(jpeg.width, 72)
		self.assertEqual(jpeg.height, 36)

		# cannot get the same image without credential
		r3 = self.anon.get("https://localhost:4433" + r1.headers["Location"])
		self.assertEqual(r3.status_code, 403)

	def test_upload_png(self):
		# upload random PNG to server
		r1 = self.user1.put("https://localhost:4433/upload/sumsum/black.png", data=self.random_image(100, 120, format="png"))
		self.assertEqual(r1.status_code, 201)
		self.assertNotEqual(r1.headers["Location"], "")

		# read back the upload image
		r2 = self.user1.get("https://localhost:4433" + r1.headers["Location"])
		self.assertEqual(r2.status_code, 200)
		self.assertEqual(r2.headers["Content-type"], "image/png")

		png = Image.open(BytesIO(r2.content))

		# the size of the images should be the same
		self.assertEqual(png.format, "PNG")
		self.assertEqual(png.width, 100)
		self.assertEqual(png.height, 120)

	def test_resize_jpeg(self):
		# upload a big JPEG image to server
		r1 = self.user1.put("https://localhost:4433/upload/sumsum/big_dir/big_image.jpg", data=self.random_image(4096, 2048))
		self.assertEqual(r1.status_code, 201)
		self.assertNotEqual(r1.headers["Location"], "")

		# read back the upload image
		r2 = self.user1.get("https://localhost:4433" + r1.headers["Location"])
		self.assertEqual(r2.status_code, 200)
		self.assertEqual(r2.headers["Content-type"], "image/jpeg")
		jpeg = Image.open(BytesIO(r2.content))

		# the size of the images should be the same
		self.assertEqual(jpeg.format, "JPEG")
		self.assertEqual(jpeg.width, 2048)
		self.assertEqual(jpeg.height, 1024)

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
		# forbidden
		r1 = self.user1.put("https://localhost:4433/upload/yungyung/abc/image.jpg", data=self.random_image(32, 16))
		self.assertEqual(r1.status_code, 403)

	def test_upload_jpeg_to_other_collection(self):
		# upload to server
		r1 = self.user1.put("https://localhost:4433/upload/sumsum/some/collection/abc.jpg", data=self.random_image(300, 200))
		self.assertEqual(r1.status_code, 201)
		blob_id = r1.headers["Location"][-40:]

		# should find it in the new collection
		r2 = self.user1.get("https://localhost:4433/coll/sumsum/some/collection/")
		self.assertEqual(r2.status_code, 200)
		self.assertEqual(r2.json()["elements"][blob_id]["filename"], "abc.jpg")
		self.assertEqual(r2.json()["elements"][blob_id]["mime"], "image/jpeg")

		blob_url = "https://localhost:4433" + r1.headers["Location"]

		# delete it afterwards
		r3 = self.user1.delete(blob_url)
		self.assertEqual(r3.status_code, 204)

		# not found in collection
		r4 = self.user1.get("https://localhost:4433/coll/sumsum/some/collection/")
		self.assertEqual(r4.status_code, 200)
		self.assertFalse(blob_id in r4.json()["elements"])

	def test_set_permission(self):
		# upload to server
		r1 = self.user1.put("https://localhost:4433/upload/sumsum/some/collection/random.jpg", data=self.random_image(100, 120))
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
		self.assertEqual(self.user2.get("https://localhost:4433" + r1.headers["Location"]).status_code, 200)

		# owner set permission to shared
		self.assertEqual(self.user1.post(
			"https://localhost:4433" + r1.headers["Location"],
			data="perm=shared",
			headers={"Content-type": "application/x-www-form-urlencoded"}
		).status_code, 204)

		# other user can get the image
		r6 = self.user2.get("https://localhost:4433" + r1.headers["Location"])
		self.assertEqual(r6.status_code, 200)
		
		# owner set permission to public
		r7 = self.user1.post(
			"https://localhost:4433" + r1.headers["Location"],
			data="perm=public",
			headers={"Content-type": "application/x-www-form-urlencoded"}
		)
		self.assertEqual(r7.status_code, 204)

		# anonymous user can find it in collection
		r8 = self.anon.get("https://localhost:4433/coll/sumsum/some/collection/")
		self.assertEqual(r8.status_code, 200)
		self.assertEqual(r8.headers["Content-type"], "application/json")
		self.assertGreater(len(r8.json()["elements"]), 0)
		self.assertEqual(r8.json()["collection"], "some/collection")
		self.assertEqual(r8.json()["owner"], "sumsum")
		self.assertEqual(r8.json()["username"], "")
		self.assertEqual(r8.json()["elements"][blob_id]["filename"], "random.jpg")
		self.assertEqual(r8.json()["elements"][blob_id]["mime"], "image/jpeg")

		# owner set permission to private
		r9 = self.user1.post(
			"https://localhost:4433" + r1.headers["Location"],
			data="perm=private",
			headers={"Content-type": "application/x-www-form-urlencoded"}
		)
		self.assertEqual(r9.status_code, 204)

		# anonymous user will not find it in collection
		r10 = self.anon.get("https://localhost:4433/coll/sumsum/some/collection/")
		self.assertEqual(r10.status_code, 200)
		self.assertFalse(blob_id in r10.json()["elements"])

if __name__ == '__main__':
	unittest.main()
