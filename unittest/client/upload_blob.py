#!/usr/bin/python3
import os, sys
import requests
import unittest
from PIL import Image, ImageFilter
from io import BytesIO
import numpy
import random
import string
import time

class NormalTestCase(unittest.TestCase):
	@staticmethod
	def response_blob(response):
		return response.headers["Location"][-40:]

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

	def get_collection(self, session, owner, coll):
		time_start = time.time()
		response = session.get("https://localhost:4433/api/" + owner + "/" + coll + "/")
		external_elapse = (time.time()-time_start) * 1000000
		internal_elapse = response.json()["elapse"]

		print("elapse time = {0}us / {1}us".format(response.json()["elapse"], external_elapse))
		self.assertLessEqual(internal_elapse, external_elapse)
		self.assertEqual(response.status_code, 200)
		self.assertEqual(response.headers["Content-type"], "application/json")
		self.assertEqual(response.json()["collection"], coll)
		self.assertEqual(response.json()["owner"], owner)
		self.assertTrue("elements" in response.json())
		self.assertTrue("meta" in response.json())
		return response.json()

	def get_public_blobs(self):
		response = self.user1.get("https://localhost:4433/query/blob_set?public&json")
		self.assertEqual(response.status_code, 200)
		self.assertEqual(response.headers["Content-type"], "application/json")
		self.assertEqual(response.json()["username"], "sumsum")
		self.assertTrue("elements" in response.json())
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
		self.assertEqual(login_response.status_code, 204)
		self.assertNotEqual(self.user1.cookies.get("id"), "")

		self.user2 = requests.Session()
		self.user2.verify = "../../etc/hearty_rabbit/certificate.pem"
		login_response = self.user2.post(
			"https://localhost:4433/login",
			data="username=siuyung&password=rabbit",
			headers={"Content-type": "application/x-www-form-urlencoded"}
		)
		self.assertEqual(login_response.status_code, 204)
		self.assertNotEqual(self.user1.cookies.get("id"), "")

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
		r1 = self.user1.put("https://localhost:4433/upload/sumsum/test.jpg", data=self.random_image(1024, 768))
		self.assertEqual(r1.status_code, 201)
		self.assertNotEqual(r1.headers["Location"], "")

		# read back the upload image
		r2 = self.user1.get("https://localhost:4433" + r1.headers["Location"])
		self.assertEqual(r2.status_code, 200)
		self.assertEqual(r2.headers["Content-type"], "image/jpeg")
		jpeg = Image.open(BytesIO(r2.content))

		# the size of the images should be the same
		self.assertEqual(jpeg.format, "JPEG")
		self.assertEqual(jpeg.width, 1024)
		self.assertEqual(jpeg.height, 768)

		# cannot get the same image without credential
		r3 = self.anon.get("https://localhost:4433" + r1.headers["Location"])
		self.assertEqual(r3.status_code, 403)

		# query the blob
		r4 = self.user1.get("https://localhost:4433/query/blob?id=" + self.response_blob(r1))
		self.assertEqual(r4.status_code, 200)
		self.assertEqual(r4.headers["Content-type"], "image/jpeg")
		jpeg4 = Image.open(BytesIO(r4.content))

		# the size of the images should be the same
		self.assertEqual(jpeg4.format, "JPEG")
		self.assertEqual(jpeg4.width, 1024)
		self.assertEqual(jpeg4.height, 768)

	def test_upload_png(self):
		# upload random PNG to server
		r1 = self.user1.put("https://localhost:4433/upload/sumsum/black.png", data=self.random_image(800, 600, format="png"))
		self.assertEqual(r1.status_code, 201)
		self.assertNotEqual(r1.headers["Location"], "")

		# read back the upload image
		r2 = self.user1.get("https://localhost:4433" + r1.headers["Location"])
		self.assertEqual(r2.status_code, 200)
		self.assertEqual(r2.headers["Content-type"], "image/png")

		png = Image.open(BytesIO(r2.content))

		# the size of the images should be the same
		self.assertEqual(png.format, "PNG")
		self.assertEqual(png.width, 800)
		self.assertEqual(png.height, 600)

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

		# read back some renditions of the upload image
		self.assertEqual(self.user1.get("https://localhost:4433" + r1.headers["Location"] + "?rendition=2048x2048").status_code, 200)

		# the master rendition should have the same width and height
		r3 = self.user1.get("https://localhost:4433" + r1.headers["Location"] + "?rendition=master")
		self.assertEqual(r3.status_code, 200)

		master = Image.open(BytesIO(r3.content))
		self.assertEqual(master.width, 4096)
		self.assertEqual(master.height, 2048)

		# generated thumbnail should be less than 768x768
		r4 = self.user1.get("https://localhost:4433" + r1.headers["Location"] + "?rendition=thumbnail")
		self.assertEqual(r4.status_code, 200)

		thumb = Image.open(BytesIO(r4.content))
		self.assertLessEqual(thumb.width, 768)
		self.assertLessEqual(thumb.height, 768)

		# query the thumbnail by the blob ID
		r5 = self.user1.get("https://localhost:4433/query/blob?rendition=thumbnail&id=" + self.response_blob(r1))
		self.assertEqual(r5.status_code, 200)

		thumb = Image.open(BytesIO(r5.content))
		self.assertLessEqual(thumb.width, 768)
		self.assertLessEqual(thumb.height, 768)


	def test_lib(self):
		# resource not exist
		self.assertEqual(self.user1.get("https://localhost:4433/lib/logo.svg").status_code, 200)
		self.assertEqual(self.anon.get("https://localhost:4433/lib/hearty_rabbit.css").status_code, 200)
		self.assertEqual(self.user2.get("https://localhost:4433/lib/hearty_rabbit.js").status_code, 200)

	def test_not_found(self):
		# resource not exist
		r1 = self.user1.get("https://localhost:4433/index.html")
		self.assertEqual(r1.status_code, 404)

		# Is all-zero blob ID really invalid? <- Yes, it's valid.
		# Requesting with a valid object ID on an object that doesn't exists will return "Not Found"
		r2 = self.user1.get("https://localhost:4433/api/sumsum/0000000000000000000000000000000000000000")
		self.assertEqual(r2.status_code, 404)

		# other user's blob: no matter the target exists or not will give you "forbidden"
		r3 = self.user1.get("https://localhost:4433/api/nestal/0100000000000000000000000000000000000003")
		self.assertEqual(r3.status_code, 403)

		# 10-digit blob ID is really invalid: it will be treated as collection name
		r4 = self.user1.get("https://localhost:4433/api/sumsum/FF0000000000000000FF")
		self.assertEqual(r4.status_code, 200)

		# invalid blob ID with funny characters: it will be treated as collection name
		r5 = self.user1.get("https://localhost:4433/api/nestal/0L00000000000000000PP0000000000000000003")
		self.assertEqual(r5.status_code, 200)

	def test_view_collection(self):
		elements = self.get_collection(self.user1, "sumsum", "")
		self.assertEqual(elements["username"], "sumsum")

	def test_upload_to_other_users_collection(self):
		# forbidden
		r1 = self.user1.put("https://localhost:4433/upload/yungyung/abc/image.jpg", data=self.random_image(640, 640))
		self.assertEqual(r1.status_code, 403)

	def test_upload_jpeg_to_other_collection(self):
		# upload to server
		r1 = self.user1.put("https://localhost:4433/upload/sumsum/some/collection/abc.jpg", data=self.random_image(800, 800))
		self.assertEqual(r1.status_code, 201)
		blob_id = self.response_blob(r1)

		# should find it in the new collection
		r2 = self.user1.get("https://localhost:4433/api/sumsum/some/collection/")
		self.assertEqual(r2.status_code, 200)
		self.assertEqual(r2.json()["elements"][blob_id]["filename"], "abc.jpg")
		self.assertEqual(r2.json()["elements"][blob_id]["mime"], "image/jpeg")

		blob_url = "https://localhost:4433" + r1.headers["Location"]

		# delete it afterwards
		r3 = self.user1.delete(blob_url)
		self.assertEqual(r3.status_code, 204)

		# not found in collection
		r4 = self.user1.get("https://localhost:4433/api/sumsum/some/collection/")
		self.assertEqual(r4.status_code, 200)
		self.assertFalse(blob_id in r4.json()["elements"])

	def test_move_blob(self):
		r1 = self.user1.put("https://localhost:4433/upload/sumsum/some/collection/happy%F0%9F%98%86%F0%9F%98%84.jpg", data=self.random_image(800, 600))
		self.assertEqual(r1.status_code, 201)
		blob_id = self.response_blob(r1)

		# move to another collection
		self.assertEqual(self.user1.post(
			"https://localhost:4433" + r1.headers["Location"],
			data="move=another/collection",
			headers={"Content-type": "application/x-www-form-urlencoded"}
		).status_code, 204)

		# get it from another collection successfully
		self.assertEqual(
			self.user1.get("https://localhost:4433/api/sumsum/another/collection/" + blob_id).status_code,
			200
		)

		# can't get it from original collection any more
		self.assertEqual(
			self.user1.get("https://localhost:4433/api/sumsum/some/collection/" + blob_id).status_code,
			404
		)

		# invalid post data
		self.assertEqual(self.user1.post(
			"https://localhost:4433/api/sumsum/another/collection/" + blob_id,
			data="invalid=parameters",
			headers={"Content-type": "application/x-www-form-urlencoded"}
		).status_code, 400)

		# invalid content type
		self.assertEqual(self.user1.post(
			"https://localhost:4433/api/sumsum/another/collection/" + blob_id,
			data="invalid=parameters",
			headers={"Content-type": "multipart/form-data; boundary=something"}
		).status_code, 400)

	def test_set_permission(self):
		# upload to server
		r1 = self.user1.put("https://localhost:4433/upload/sumsum/some/collection/random%F0%9F%98%8A.jpg", data=self.random_image(1000, 1200))
		self.assertEqual(r1.status_code, 201)
		blob_id = self.response_blob(r1)

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
		self.assertTrue(blob_id in self.get_collection(self.user2, "sumsum", "some/collection")["elements"])

		# new blob can be found in the public list
		self.assertTrue(blob_id in self.get_public_blobs()["elements"].keys())

		# anonymous user can find it in collection
		self.assertEqual(self.anon.get("https://localhost:4433" + r1.headers["Location"]).status_code, 200)
		self.assertTrue(blob_id in self.get_collection(self.anon,  "sumsum", "some/collection")["elements"])

		# anonymous user can query the blob
		self.assertEqual(self.anon.get("https://localhost:4433/query/blob?id=" + blob_id).status_code, 200)

		# owner set permission to shared
		self.assertEqual(self.user1.post(
			"https://localhost:4433" + r1.headers["Location"],
			data="perm=shared",
			headers={"Content-type": "application/x-www-form-urlencoded"}
		).status_code, 204)

		# other user can get the image
		self.assertEqual(self.user2.get("https://localhost:4433" + r1.headers["Location"]).status_code, 200)
		self.assertTrue(blob_id in self.get_collection(self.user2,  "sumsum", "some/collection")["elements"])

		# anonymous user cannot
		self.assertEqual(self.anon.get("https://localhost:4433" + r1.headers["Location"]).status_code, 403)
		self.assertFalse(blob_id in self.get_collection(self.anon,  "sumsum", "some/collection")["elements"])

		# new blob can no longer be found in the public list
		self.assertFalse(blob_id in self.get_public_blobs()["elements"].keys())

	def test_scan_collections(self):
		covers = {}

		# upload random image to 10 different collections
		for x in range(10):
			upload = "https://localhost:4433/upload/sumsum/collection{}/random.jpg".format(x)
			res = self.user1.put(upload + "", data=self.random_image(480, 480))
			self.assertEqual(res.status_code, 201)

			cover = self.response_blob(res)
			covers[x] = cover

			# set the cover to the newly added image
			view = "https://localhost:4433/api/sumsum/collection{}/".format(x)
			self.assertEqual(self.user1.post(view,
				data="cover=" + cover,
				headers={"Content-type": "application/x-www-form-urlencoded"}
			).status_code, 204)

		r1 = self.user1.get("https://localhost:4433/query/collection?user=sumsum&json")
		self.assertEqual(r1.status_code, 200)
		self.assertTrue("colls" in r1.json())
		self.assertTrue("collection1" in r1.json()["colls"])

		for x in range(10):
			self.assertEqual(r1.json()["colls"]["collection{}".format(x)]["cover"], covers[x])

		# error case: try setting the covers using images from other collections
		for x in range(10):
			comp = 9 - x
			if comp != x:
				view = "https://localhost:4433/api/sumsum/collection{}/".format(x)
				self.assertEqual(self.user1.post(view,
					data="cover=" + covers[comp],
					headers={"Content-type": "application/x-www-form-urlencoded"}
				).status_code, 400)

	def test_session_expired(self):
		old_session = self.user1.cookies["id"]
		self.user1.get("https://localhost:4433/logout")

		# should give 404 instead of 403 if still login
		self.assertEqual(
			self.user1.get("https://localhost:4433/api/sumsum/0000000000000000000000000000000000000000").status_code,
			403
		)

		# reuse old cookie to simulate session expired
		self.user1.cookies["id"] = old_session
		r1 = self.user1.get("https://localhost:4433/api/sumsum/0000000000000000000000000000000000000000")
		self.assertEqual(r1.status_code, 403)

	def test_login_incorrect(self):
		session = requests.Session()
		session.verify = "../../etc/hearty_rabbit/certificate.pem"

		login_response = session.post(
			"https://localhost:4433/login",
			data="username=invalid&password=invalid",
			headers={"Content-type": "application/x-www-form-urlencoded"}
		)
		self.assertEqual(login_response.status_code, 403)
		self.assertEqual(session.cookies.get("id"), None)
		session.close()

	def test_percent_collection(self):
		# upload a random image to a collection with a Japanese name
		r1 = self.user1.put(
			"https://localhost:4433/upload/sumsum/%E5%A5%B3%E7%A5%9E%E3%83%8F%E3%82%A4%E3%83%AA%E3%82%A2/test.jpg",
			data=self.random_image(1200, 1000)
		)
		self.assertEqual(r1.status_code, 201)

		r2 = self.user1.get("https://localhost:4433/query/collection?user=sumsum&json")
		self.assertEqual(r2.status_code, 200)
		self.assertTrue("colls" in r2.json())
		self.assertTrue("女神ハイリア" in r2.json()["colls"])

	def test_percent_filename(self):
		r1 = self.user1.put(
			"https://localhost:4433/upload/sumsum/%E3%83%8F%E3%82%A4%E3%83%AA%E3%82%A2%E3%81%AE%E7%9B%BE/%E9%A3%9F%E5%93%82%E5%95%B2%E7%94%98%E8%8D%80%3F_carrot.jpg",
			data=self.random_image(1200, 1000)
		)
		self.assertEqual(r1.status_code, 201)
		blob_id = self.response_blob(r1)

		# should find it in the new collection
		r2 = self.user1.get("https://localhost:4433/api/sumsum/%E3%83%8F%E3%82%A4%E3%83%AA%E3%82%A2%E3%81%AE%E7%9B%BE/?json")
		self.assertEqual(r2.status_code, 200)
		self.assertEqual(r2.json()["elements"][blob_id]["filename"], "食哂啲甘荀?_carrot.jpg")
		self.assertEqual(r2.json()["elements"][blob_id]["mime"], "image/jpeg")
		self.assertEqual("sumsum", r2.json()["username"])
		self.assertEqual("ハイリアの盾", r2.json()["collection"])

	def test_remove_cover(self):
		# delete all images in test_cover_album
		r0 = self.user1.get("https://localhost:4433/api/sumsum/%F0%9F%99%87/")
		self.assertEqual(r0.status_code, 200)
		for blob in r0.json()["elements"].keys():
			self.assertEqual(self.user1.delete("https://localhost:4433/api/sumsum/%F0%9F%99%87/" + blob).status_code, 204)

		# upload one image, and it will become the cover of the album
		r1 = self.user1.put(
			"https://localhost:4433/upload/sumsum/%F0%9F%99%87/cover.jpg",
			data=self.random_image(1000, 1000)
		)
		self.assertEqual(r1.status_code, 201)
		cover_id = self.response_blob(r1)
		self.assertEqual(cover_id, cover_id.lower())

		# verify the first image will become the cover of the album
		r2 = self.user1.get("https://localhost:4433/query/collection?user=sumsum&json")
		self.assertEqual(r2.status_code, 200)
		self.assertTrue("🙇" in r2.json()["colls"])
		self.assertEqual(cover_id, r2.json()["colls"]["🙇"]["cover"])
		self.assertEqual("sumsum", r2.json()["username"])

		# upload another image, but the cover will stay the same
		r3 = self.user1.put(
			"https://localhost:4433/upload/sumsum/%F0%9F%99%87/not_cover.jpg",
			data=self.random_image(700, 700)
		)
		self.assertEqual(r3.status_code, 201)
		second_image = self.response_blob(r3)
		self.assertEqual(second_image, second_image.lower())

		# verify that the cover will stay the same
		r4 = self.user1.get("https://localhost:4433/query/collection?user=sumsum&json")
		self.assertEqual(r4.status_code, 200)
		self.assertEqual(cover_id, r4.json()["colls"]["🙇"]["cover"])
		self.assertEqual("sumsum", r4.json()["username"])

		# delete the cover image
		self.assertEqual(self.user1.delete("https://localhost:4433/api/sumsum/%F0%9F%99%87/" + cover_id).status_code, 204)

		# the cover will become the second image
		r5 = self.user1.get("https://localhost:4433/query/collection?user=sumsum&json")
		self.assertEqual(r5.status_code, 200)
		self.assertTrue("🙇" in r5.json()["colls"])
		self.assertEqual(second_image, r5.json()["colls"]["🙇"]["cover"])

		# delete the other image as well
		self.assertEqual(self.user1.delete("https://localhost:4433" + r3.headers["Location"]).status_code, 204)

		# the album will be removed
		r6 = self.user1.get("https://localhost:4433/query/collection?user=sumsum&json")
		self.assertEqual(r6.status_code, 200)
		self.assertFalse("🙇" in r6.json()["colls"])

	def test_share_link(self):
		# upload to default album
		up1 = self.user1.put("https://localhost:4433/upload/sumsum/new.jpg", data=self.random_image(1000, 1200))
		new_blob = self.response_blob(up1)

		# set permission to shared
		perm = self.user1.post(
			"https://localhost:4433" + up1.headers["Location"],
			data="perm=shared",
			headers={"Content-type": "application/x-www-form-urlencoded"}
		)
		self.assertEqual(perm.status_code, 204)

		# share link of default album
		slink = self.user1.post("https://localhost:4433/api/sumsum/", data="share=create",
			headers={"Content-type": "application/x-www-form-urlencoded"}
		)
		self.assertEqual(slink.status_code, 204)
		self.assertNotEqual(slink.headers["Location"], "")

		# list all shared links
		slist = self.user1.post("https://localhost:4433/api/sumsum/", data="share=list",
			headers={"Content-type": "application/x-www-form-urlencoded"}
		)
		self.assertEqual(slist.status_code, 200)
#		self.assertTrue(auth_key in slist.json())
		print(slist.json())

		# anonymous user can fetch the shared link
		view_slink = self.anon.get(("https://localhost:4433" + slink.headers["Location"]).replace("/view", "/api"))
		self.assertNotEqual(self.anon.cookies.get("id"), "")
		self.assertEqual(view_slink.status_code, 200)
		self.assertTrue("auth" in view_slink.json())
		self.assertTrue(new_blob in view_slink.json()["elements"])

		# verify auth key
		auth_key = view_slink.json()["auth"]
		self.assertEqual(slink.headers["Location"][-len(auth_key):], auth_key)

		# the same auth key does not work for other collections
		other_response = self.anon.get("https://localhost:4433/api/sumsum/other/?auth=" + auth_key)
		self.assertNotEqual(self.anon.cookies.get("id"), "")
		self.assertFalse(new_blob in other_response.json()["elements"])

		somecoll_response = self.anon.get("https://localhost:4433/api/sumsum/some/collection/?auth=" + auth_key)
		self.assertNotEqual(self.anon.cookies.get("id"), "")
		self.assertFalse(new_blob in somecoll_response.json()["elements"])

		# the auth key can't be used for upload
		up2 = self.anon.put("https://localhost:4433/upload/sumsum/new.jpg?auth=" + auth_key, data=self.random_image(1000, 1200))
		self.assertNotEqual(self.anon.cookies.get("id"), "")
		self.assertEqual(up2.status_code, 400)

	def test_invalid_auth_key_treat_as_public(self):
		# upload to album
		up1 = self.user1.put("https://localhost:4433/upload/sumsum/test_invalid_auth_key/new.jpg", data=self.random_image(1000, 1200))
		new_blob = self.response_blob(up1)

		# set permission to shared
		self.assertEqual(
			self.user1.post(
				"https://localhost:4433" + up1.headers["Location"],
				data="perm=shared",
				headers={"Content-type": "application/x-www-form-urlencoded"}
			).status_code,
			204
		)

		# try to read the album with an invalid key
		invalid_auth_key = "".join(random.choices(string.hexdigits, k=40))
		dir_api = self.anon.get("https://localhost:4433/api/sumsum/test_invalid_auth_key/?auth=" + invalid_auth_key)
		self.assertEqual(dir_api.status_code, 200)
		self.assertFalse("auth" in dir_api.json())
		self.assertFalse(new_blob in dir_api.json()["elements"])

		# set permission to public
		self.assertEqual(
			self.user1.post(
				"https://localhost:4433" + up1.headers["Location"],
				data="perm=public",
				headers={"Content-type": "application/x-www-form-urlencoded"}
			).status_code,
			204
		)

		# can see public image
		dir_api2 = self.anon.get("https://localhost:4433/api/sumsum/test_invalid_auth_key/?auth=" + invalid_auth_key)
		self.assertEqual(dir_api2.status_code, 200)
		self.assertFalse("auth" in dir_api2.json())
		self.assertTrue(new_blob in dir_api2.json()["elements"])

if __name__ == '__main__':
	unittest.main()
