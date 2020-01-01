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
		lena = Image.open("../common/lena.png").convert("RGBA").resize((width, height), Image.ANTIALIAS)

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
		self.assertEqual(self.user1.user(), "sumsum")

		self.user2 = hrb.Session(self.m_site)
		self.user2.login("siuyung", "rabbit")
		self.assertEqual(self.user2.user(), "siuyung")

		self.anon = hrb.Session(self.m_site)
		self.assertEqual(self.anon.user(), None)

	def tearDown(self):
		self.user1.close()
		self.user2.close()
		self.anon.close()

	def test_login_incorrect(self):
		self.assertRaises(hrb.Forbidden, self.anon.login, "sumsum", "rabbit")

	def test_upload_jpeg(self):
		id = self.user1.upload("test_api", "test_lena.jpg", self.random_image(1024, 768))
		self.assertEqual(len(id), 40)

		# Collection "test_api" is created
		coll_list = self.user1.list_collections()
		self.assertEqual(self.user1.find_collection("test_api").owner(), "sumsum")

		# get the blob we just uploaded
		lena1 = self.user1.get_blob("test_api", id)
		self.assertEqual(lena1.mime(), "image/jpeg")
		self.assertEqual(lena1.filename(), "test_lena.jpg")
		jpeg = Image.open(BytesIO(lena1.data()))

		# the size of the images should be the same
		self.assertEqual(jpeg.format, "JPEG")
		self.assertEqual(jpeg.width, 1024)
		self.assertEqual(jpeg.height, 768)

		# cannot get the same image without credential
		self.assertRaises(hrb.NotFound, self.anon.get_blob, "test_api", id)

		# query the blob. It should be the same as the one we just get
		query = self.user1.query_blob(id)
		self.assertEqual(query.mime(), "image/jpeg")
		self.assertEqual(query.data(), lena1.data())

		# The newly uploaded image should be in the "test_api" collection
		self.assertTrue(id in self.user1.list_blobs("test_api"))

	def test_upload_png(self):
		# upload random PNG to server
		blobid = self.user1.upload("", "black.png", data=self.random_image(800, 600, format="png"))
		self.assertEqual(len(blobid), 40)

		# read back the upload image
		blob = self.user1.get_blob("", blobid)
		self.assertEqual(blob.mime(), "image/png")

		png = Image.open(BytesIO(blob.data()))

		# the size of the images should be the same
		self.assertEqual(png.format, "PNG")
		self.assertEqual(png.width, 800)
		self.assertEqual(png.height, 600)

	def test_resize_jpeg(self):
		# upload a big JPEG image to server
		blobid = self.user1.upload("big_dir", "big_image.jpg", data=self.random_image(4096, 2048))

		# read back the upload image
		blob = self.user1.get_blob("big_dir", blobid)
		self.assertEqual(blob.mime(), "image/jpeg")
		jpeg = Image.open(BytesIO(blob.data()))

		# the size of the images should be the same
		self.assertEqual(jpeg.format, "JPEG")
		self.assertEqual(jpeg.width, 2048)
		self.assertEqual(jpeg.height, 1024)

		# read back some renditions of the upload image
		self.assertEqual(self.user1.get_blob("big_dir", blobid, rendition="2048x2048").id(), blobid)

		# the master rendition should have the same width and height
		master = self.user1.get_blob("big_dir", blobid, rendition="master")
		self.assertEqual(master.filename(), "big_image.jpg")

		master = Image.open(BytesIO(master.data()))
		self.assertEqual(master.width, 4096)
		self.assertEqual(master.height, 2048)

		# generated thumbnail should be less than 768x768
		thumb = self.user1.get_blob("big_dir", blobid, rendition="thumbnail")

		thumb = Image.open(BytesIO(thumb.data()))
		self.assertLessEqual(thumb.width, 768)
		self.assertLessEqual(thumb.height, 768)

		# query the thumbnail by the blob ID
		r5 = self.user1.query_blob(blobid, rendition="thumbnail")

		thumb = Image.open(BytesIO(r5.data()))
		self.assertLessEqual(thumb.width, 768)
		self.assertLessEqual(thumb.height, 768)

	def test_lib(self):
		self.assertEqual(self.user1.m_session.get("https://localhost:4433/lib/logo.svg").status_code, 200)
		self.assertEqual(self.anon.m_session.get("https://localhost:4433/lib/hearty_rabbit.css").status_code, 200)
		self.assertEqual(self.user2.m_session.get("https://localhost:4433/lib/hearty_rabbit.js").status_code, 200)

		# resource not exist
		self.assertEqual(self.user1.m_session.get("https://localhost:4433/lib/index.html").status_code, 404)
		self.assertEqual(self.anon.m_session.get("https://localhost:4433/lib/login.html").status_code, 404)


	def test_not_found(self):
		# resource not exist
		r1 = self.user1.m_session.get("https://localhost:4433/index.html")
		self.assertEqual(r1.status_code, 404)

		# Is all-zero blob ID really invalid? <- Yes, it's valid.
		# Requesting with a valid object ID on an object that doesn't exists will return "Not Found"
		self.assertRaises(hrb.NotFound, self.user1.get_blob, "", "0000000000000000000000000000000000000000")

		# other user's blob: no matter the target exists or not will give you "forbidden"
		r3 = self.user1.m_session.get("https://localhost:4433/api/nestal/0100000000000000000000000000000000000003")
		self.assertEqual(r3.status_code, 403)

		# 10-digit blob ID is really invalid: it will be treated as collection name
		r4 = self.user1.m_session.get("https://localhost:4433/api/sumsum/FF0000000000000000FF")
		self.assertEqual(r4.status_code, 200)

		# invalid blob ID with funny characters: it will be treated as collection name
		r5 = self.user1.m_session.get("https://localhost:4433/api/nestal/0L00000000000000000PP0000000000000000003")
		self.assertEqual(r5.status_code, 200)

	def test_upload_to_other_users_collection(self):
		self.user1.m_user = "yungyung"
		self.assertRaises(hrb.Forbidden, self.user1.upload, "abc", "image.jpg", data=self.random_image(640, 640))

		self.anon.m_user = "sumsum"
		self.assertRaises(hrb.BadRequest, self.anon.upload, "abc", "image.jpg", data=self.random_image(640, 640))

	def test_upload_jpeg_to_other_collection(self):
		# upload to server
		blob_id = self.user1.upload("some/collection", "abc.jpg", data=self.random_image(800, 800))

		# should find it in the new collection
		r2 = self.user1.list_blobs("some/collection")
		self.assertTrue(blob_id in r2)
		abc = r2[blob_id]
		self.assertEqual(abc.filename(), "abc.jpg")
		self.assertEqual(abc.mime(), "image/jpeg")
		self.assertTrue(abc.mime() is not None)

		# delete it afterwards
		self.user1.delete_blob("some/collection", blob_id)

		# not found in collection
		r4 = self.user1.list_blobs("some/collection")
		self.assertFalse(blob_id in r4)


	def test_move_blob(self):
		blob_id = self.user1.upload("some/collection", "happy😆faces😄.jpg", data=self.random_image(800, 600))

		# move to another collection
		self.user1.move_blob("some/collection", blob_id, "another/collection")

		# get it from another collection successfully
		self.assertEqual(self.user1.get_blob("another/collection", blob_id).filename(), "happy😆faces😄.jpg")

		# can't get it from original collection any more
		self.assertRaises(hrb.NotFound, self.user1.get_blob, "some/collection", blob_id)

		# another collection is created
		dest_coll = self.user1.list_blobs("another/collection")
		self.assertTrue(blob_id in dest_coll)
		self.assertEqual(dest_coll[blob_id].filename(), "happy😆faces😄.jpg")
		self.assertEqual(self.user1.find_collection("another/collection").owner(), "sumsum")

		# invalid post data
		self.assertEqual(self.user1.m_session.post(
			"https://localhost:4433/api/sumsum/another/collection/" + blob_id,
			data="invalid=parameters",
			headers={"Content-type": "application/x-www-form-urlencoded"}
		).status_code, 400)

		# invalid content type
		self.assertEqual(self.user1.m_session.post(
			"https://localhost:4433/api/sumsum/another/collection/" + blob_id,
			data="invalid=parameters",
			headers={"Content-type": "multipart/form-data; boundary=something"}
		).status_code, 400)

	def test_set_permission(self):
		# upload to server
		blob_id = self.user1.upload("some/collection", "派石😊.jpg", data=self.random_image(1000, 1200))

		# owner get successfully
		self.assertEqual(self.user1.get_blob("some/collection", blob_id).filename(), "派石😊.jpg")

		# owner set permission to public
		self.user1.set_permission("some/collection", blob_id, "public")

		# other user can get the image
		self.assertEqual(self.user2.get_blob("some/collection", blob_id, self.user1.user()).id(), blob_id)

		# new blob can be found in the public list from sumsum
		self.assertTrue(blob_id in self.user2.list_public_blobs())
		self.assertTrue(blob_id in self.user2.list_public_blobs(user="sumsum"))

		# user "unknown" has not yet uploaded any public blobs
		self.assertEqual(len(self.user2.list_public_blobs(user="unknown")), 0)

		# anonymous user can find it in collection
		self.assertEqual(self.anon.get_blob("some/collection", blob_id, self.user1.user()).id(), blob_id)
		self.assertTrue(blob_id in self.anon.list_blobs("some/collection", user="sumsum"))

		# anonymous user can query the blob
		# TODO: allow anonymous user to query public blob
		# TODO: add "query_blob_ref" to support querying which collection has the blob
		# self.assertEqual(self.anon.query_blob(blob_id)["id"], blob_id)

		# owner set permission to shared
		self.user1.set_permission("some/collection", blob_id, "shared")

		# other user can get the image
		self.assertEqual(self.user2.get_blob("some/collection", blob_id, "sumsum").id(), blob_id)
		self.assertTrue(blob_id in self.user2.list_blobs("some/collection", "sumsum"))

		# anonymous user cannot
		self.assertRaises(hrb.Forbidden, self.anon.get_blob, "some/collection", blob_id, "sumsum")
		self.assertFalse(blob_id in self.anon.list_blobs("some/collection", "sumsum"))

		# new blob can no longer be found in the public list
		self.assertFalse(blob_id in self.user1.list_public_blobs())

	def test_scan_collections(self):
		covers = {}

		# upload random image to 10 different collections
		for x in range(10):
			coll = "collection{}".format(x)
			covers[x] = self.user1.upload(coll, "random.jpg", data=self.random_image(480, 480))

			# set the cover to the newly added image
			self.user1.set_cover(coll, covers[x])

		colls = self.user1.list_collections()
		self.assertEqual(next(x for x in colls if x.name() == "collection1").cover(), covers[1])

		for x in range(10):
			self.assertEqual(next(i for i in colls if i.name() == "collection{}".format(x)).cover(), covers[x])

		# error case: try setting the covers using images from other collections
		for x in range(10):
			comp = 9 - x
			if comp != x:
				self.assertRaises(hrb.BadRequest, self.user1.set_cover, "sumsum/collection{}".format(x), covers[comp])

	def test_session_expired(self):
		old_session = self.user1.m_session.cookies["id"]
		self.user1.logout()

		# should give 404 instead of 403 if still login
		self.assertRaises(hrb.Forbidden, self.user1.get_blob, "", "0000000000000000000000000000000000000000")

		# reuse old cookie to simulate session expired
		self.user1.m_session.cookies["id"] = old_session
		self.assertRaises(hrb.Forbidden, self.user1.get_blob, "", "0000000000000000000000000000000000000000")

	def test_japanese(self):
		# upload a random image to a collection with a Japanese name
		self.user1.upload("女神ハイリア", "test.jpg", data=self.random_image(1200, 1000))
		self.assertTrue("女神ハイリア", self.user1.list_collections())

		# upload a random image with Japanese filename
		blobid = self.user1.upload("No.5849", "初雪の大魔女・リーチェ.jpg", data=self.random_image(1200, 1000))
		self.assertEqual(self.user1.get_blob("No.5849", blobid).filename(), "初雪の大魔女・リーチェ.jpg")

	def test_percent_filename(self):
		blobid = self.user1.upload("神座之靈央神", "食哂啲甘荀?_carrot.jpg", data=self.random_image(1200, 1000))

		# should find it in the new collection
		coll = self.user1.list_blobs("神座之靈央神")
		self.assertTrue(blobid in coll)
		self.assertEqual(coll[blobid].filename(), "食哂啲甘荀?_carrot.jpg")
		self.assertEqual(coll[blobid].mime(), "image/jpeg")


if __name__ == '__main__':
	unittest.main()
