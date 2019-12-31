#!/usr/bin/python3

import unittest
from PIL import Image, ImageFilter
from io import BytesIO
import numpy
import random
import string
import time

import importlib.util
spec = importlib.util.spec_from_file_location("hrb", "../../client/python/HeartyRabbit.py")
hrb = importlib.util.module_from_spec(spec)
spec.loader.exec_module(hrb)

class NormalTestCase(unittest.TestCase):

	m_site = "localhost:4433"
	user1 = None
	user2 = None
	anon  = None

	# without the "test" prefix in the method, it is not treated as a test routine
	@staticmethod
	def random_image(width, height, format="jpeg"):
		lena = Image.open("../tests/image/lena.png").convert("RGBA").resize((width, height), Image.ANTIALIAS)

		# random noise
		random_array = numpy.random.rand(128, 128, 4) * 255
		noise = Image.fromarray(random_array.astype("uint8")).convert("RGBA").resize((width, height))
		noise = noise.filter(ImageFilter.GaussianBlur(4))

		# blend original image with noise
		result = Image.alpha_composite(lena, noise).convert("RGB")

		# save image to buffer
		temp = BytesIO()
		result.save(temp, format=format, quality=50)
		return temp.getvalue()

	def setUp(self):
		# set up a session with valid credential
		self.user1 = hrb.Session(self.m_site)
		self.user1.login("sumsum", "bearbear")
		self.user2 = hrb.Session(self.m_site)
		self.user2.login("siuyung", "rabbit")

		self.anon = hrb.Session(self.m_site)

	def tearDown(self):
		self.user1.close()
		self.user2.close()
		self.anon.close()

	def test_login_incorrect(self):
		bad_session = hrb.Session(self.m_site)
		self.assertRaises(PermissionError, bad_session.login, "sumsum", "rabbit")
		bad_session.close()

	def test_upload_jpeg(self):
		id = self.user1.upload("test_api", "test_lena.jpg", self.random_image(1024, 768))
		self.assertEqual(len(id), 40)

		# get the blob we just uploaded
		lena1 = self.user1.get_blob("test_api", id)
		self.assertEqual(lena1["mime"], "image/jpeg")
		jpeg = Image.open(BytesIO(lena1["data"]))

		# the size of the images should be the same
		self.assertEqual(jpeg.format, "JPEG")
		self.assertEqual(jpeg.width, 1024)
		self.assertEqual(jpeg.height, 768)

		# cannot get the same image without credential
		self.assertRaises(FileNotFoundError, self.anon.get_blob, "test_api", id)

		# query the blob. It should be the same as the one we just get
		query = self.user1.query_blob(id)
		self.assertEqual(query["mime"], "image/jpeg")
		self.assertEqual(query["data"], lena1["data"])

		# The newly uploaded image should be in the "test_api" collection
		colls = self.user1.list_blobs("test_api")
		self.assertEqual(len([x for x in colls if x.id() == id]), 1)

	def test_upload_png(self):
		# upload random PNG to server
		id = self.user1.upload("", "black.png", data=self.random_image(800, 600, format="png"))
		self.assertEqual(len(id), 40)

		# read back the upload image
		r2 = self.user1.get_blob("", id)
		self.assertEqual(r2["mime"], "image/png")

		png = Image.open(BytesIO(r2["data"]))

		# the size of the images should be the same
		self.assertEqual(png.format, "PNG")
		self.assertEqual(png.width, 800)
		self.assertEqual(png.height, 600)


	def test_resize_jpeg(self):
		# upload a big JPEG image to server
		r1 = self.user1.upload("big_dir", "big_image.jpg", data=self.random_image(4096, 2048))
		self.assertEqual(len(r1), 40)

		# read back the upload image
		r2 = self.user1.get_blob("big_dir", r1)
		self.assertEqual(r2["mime"], "image/jpeg")
		jpeg = Image.open(BytesIO(r2["data"]))

		# the size of the images should be the same
		self.assertEqual(jpeg.format, "JPEG")
		self.assertEqual(jpeg.width, 2048)
		self.assertEqual(jpeg.height, 1024)

		# read back some renditions of the upload image
		self.assertEqual(self.user1.get_blob("big_dir", r1, rendition="2048x2048")["id"], r1)

		# the master rendition should have the same width and height
		r3 = self.user1.get_blob("big_dir", r1, rendition="master")

		master = Image.open(BytesIO(r3["data"]))
		self.assertEqual(master.width, 4096)
		self.assertEqual(master.height, 2048)

		# generated thumbnail should be less than 768x768
		r4 = self.user1.get_blob("big_dir", r1, rendition="thumbnail")
		self.assertEqual(len(r4["id"]), 40)

		thumb = Image.open(BytesIO(r4["data"]))
		self.assertLessEqual(thumb.width, 768)
		self.assertLessEqual(thumb.height, 768)

		# query the thumbnail by the blob ID
		r5 = self.user1.query_blob(r1, rendition="thumbnail")
		self.assertEqual(len(r4["id"]), 40)

		thumb = Image.open(BytesIO(r5["data"]))
		self.assertLessEqual(thumb.width, 768)
		self.assertLessEqual(thumb.height, 768)

	def test_lib(self):
		self.assertEqual(self.user1.m_session.get("https://localhost:4433/lib/logo.svg").status_code, 200)
		self.assertEqual(self.anon.m_session.get("https://localhost:4433/lib/hearty_rabbit.css").status_code, 200)
		self.assertEqual(self.user2.m_session.get("https://localhost:4433/lib/hearty_rabbit.js").status_code, 200)

		# resource not exist
		self.assertEqual(self.user1.m_session.get("https://localhost:4433/lib/index.html").status_code, 404)
		self.assertEqual(self.anon.m_session.get("https://localhost:4433/lib/login.html").status_code, 404)


if __name__ == '__main__':
	unittest.main()
