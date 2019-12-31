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

	def tearDown(self):
		self.user1.close()

	def test_login_incorrect(self):
		bad_session = hrb.Session(self.m_site)
		self.assertRaises(PermissionError, bad_session.login, "sumsum", "rabbit")

	def test_upload_jpeg(self):
		image = self.random_image(1024, 768)
		id = self.user1.upload("test_api", "test_lena.jpg", image)
		self.assertNotEqual(id, "")

		lena1 = self.user1.get_blob("test_api", id)
		self.assertEqual(lena1["mime"], "image/jpeg")
		jpeg = Image.open(BytesIO(lena1["data"]))

		# the size of the images should be the same
		self.assertEqual(jpeg.format, "JPEG")
		self.assertEqual(jpeg.width, 1024)
		self.assertEqual(jpeg.height, 768)

if __name__ == '__main__':
	unittest.main()
