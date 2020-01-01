import requests
import urllib.parse
import os
import re

class HrbException(Exception):
	pass


class Forbidden(HrbException):
	pass


class NotFound(HrbException):
	pass


class BadRequest(HrbException):
	pass


class Blob:
	m_json = None
	m_id = None

	def __init__(self, id, json):
		self.m_id = id
		self.m_json = json

	def filename(self):
		return self.m_json["filename"]

	def mime(self):
		return self.m_json["mime"]

	def timestamp(self):
		return self.m_json["timestamp"]

	def id(self):
		return self.m_id


class Collection:
	m_json = None

	def __init__(self, json):
		self.m_json = json

	def name(self):
		return self.m_json["coll"]

	def cover(self):
		return self.m_json["cover"]

	def owner(self):
		return self.m_json["owner"]


class Session:
	m_site = None
	m_user = None
	m_session = None

	def __init__(self, site = "localhost:4433", cert = None):
		self.m_session = requests.Session()
		self.m_site = urllib.parse.urlparse("https://" + site)

		# This is a hack for testing purpose.
		if self.m_site.netloc == "localhost:4433" and cert is None:
			self.m_session.verify = "../../etc/hearty_rabbit/certificate.pem"

	def __url(self, path, query = None):
		url = self.m_site._replace(path = path)
		if query is not None:
			url = url._replace(query = urllib.parse.urlencode(query))

		return urllib.parse.urlunparse(url)

	@staticmethod
	def __quote_path(path):
		comp = os.path.split(path)
		return os.path.join(*[urllib.parse.quote_plus(x) for x in comp])

	def __format_url(self, action, owner, path, filename, query = None):
		return self.__url(
			"/{}/{}/{}/{}".format(
				action,
				"" if owner is None else urllib.parse.quote_plus(owner),
				"" if path is None else self.__quote_path(path),
				"" if filename is None else urllib.parse.quote_plus(filename)
			),
			query=query
		)

	def user(self):
		return self.m_user

	@staticmethod
	def raise_exception(status_code, format_string):
		message = format_string.format(status_code)
		if status_code == 404:
			raise NotFound(message)
		elif status_code == 403:
			raise Forbidden(message)
		elif status_code == 400:
			raise BadRequest(message)
		else:
			raise HrbException(message)

	def login(self, user, password):
		response = self.m_session.post(
			self.__url("/login"),
			data="username=" + user + "&password=" + password,
			headers={"Content-type": "application/x-www-form-urlencoded"}
		)

		if response.status_code != 204 or self.m_session.cookies.get("id") == "":
			self.raise_exception(response.status_code, "source site login incorrect: {0}")

		self.m_user = user

	def logout(self):
		response = self.m_session.get(self.__url("/logout"))
		if response.status_code != 200:
			self.raise_exception("cannot logout: {}", response.status_code)

	def close(self):
		self.m_session.close()

	def list_collections(self, user = None):
		if user is None:
			user = self.m_user

		response = self.m_session.get(self.__url("/query/collection", {"user": user, "json": None}))
		if response.status_code != 200:
			self.raise_exception(response.status_code, "cannot query album list: {0}")

		result = []
		for coll in response.json()["colls"]:
			result.append(Collection(coll))

		return result

	def find_collection(self, collection, user = None):
		coll_list = self.list_collections(user)
		return next((x for x in coll_list if x.name() == collection), None)

	@staticmethod
	def __elements_to_blobs(json):
		blobs = {}
		for id, blob in json["elements"].items():
			blobs[id] = Blob(id, blob)
		return blobs

	def list_blobs(self, collection, user = None):
		if user is None:
			user = self.m_user

		response = self.m_session.get(self.__format_url("api", user, collection, ""))
		if response.status_code != 200:
			self.raise_exception(response.status_code, "cannot get blobs from collections \"{}\": {}".format(
				collection,
				response.status_code
			))
		# Server should return our username and put it in the JSON, so they should match.
		# This is just a sanity check.
		# That is, unless we have not yet login (i.e. m_user is None). In that case "username" should not be
		# in the JSON.
		if response.json().get("username") == self.m_user:
			return self.__elements_to_blobs(response.json())
		else:
			return {}

	def upload(self, collection, filename, data):
		response = self.m_session.put(
			self.__format_url("upload", self.m_user, collection, filename),
			data=data
		)
		if response.status_code != 201:
			self.raise_exception(response.status_code, "cannot upload blobs to collections \"{}\": {}".format(
				collection,
				response.status_code
			))
		return response.headers["Location"][-40:]

	def get_blob(self, collection, blobid, user = None, rendition = ""):
		if user is None:
			user = self.m_user

		query = {}
		if rendition != "":
			query["rendition"] = rendition

		response = self.m_session.get(self.__format_url("api", user, collection, blobid, query=query))
		if response.status_code != 200:
			self.raise_exception(response.status_code, "cannot get blob {} from user \"{}\": {}".format(
				blobid, user, response.status_code
			))

		disposition = response.headers['content-disposition']
		fname = re.findall("filename=(.+)", disposition)[0]

		return {
			"id": blobid, "mime": response.headers["Content-type"], "data": response.content,
			"filename": urllib.parse.unquote_plus(fname)
		}

	def query_blob(self, blobid, rendition = ""):
		query = {"id": blobid}
		if rendition != "":
			query["rendition"] = rendition
		response = self.m_session.get(self.__url("/query/blob", query))
		if response.status_code != 200:
			self.raise_exception(response.status_code, "cannot query blob {}: {}".format(
				blobid, response.status_code
			))
		return {"id": blobid, "mime": response.headers["Content-type"], "data": response.content}

	def delete_blob(self, collection, blobid):
		response = self.m_session.delete(self.__format_url("api", self.m_user, collection, blobid))
		if response.status_code != 204:
			self.raise_exception(response.status_code, "cannot delete blob {}: {}".format(
				blobid, response.status_code
			))

	def move_blob(self, src, blobid, dest):
		response = self.m_session.post(
			self.__format_url("api", self.m_user, src, blobid),
			data="move=another/collection",
			headers={"Content-type": "application/x-www-form-urlencoded"}
		)
		if response.status_code != 204:
			self.raise_exception(response.status_code, "cannot move blob {} from {} to {}: {}".format(
				blobid, src, dest, response.status_code
			))

	def set_permission(self, collection, blobid, perm):
		response = self.m_session.post(
			self.__format_url("api", self.m_user, collection, blobid),
			data="perm=" + perm,
			headers={"Content-type": "application/x-www-form-urlencoded"}
		)
		if response.status_code != 204:
			self.raise_exception(response.status_code, "cannot set permission of blob {} from {} to {}: {}".format(
				blobid, collection, perm, response.status_code
			))

	def set_cover(self, collection, cover):
		response = self.m_session.post(
				self.__format_url("api", self.m_user, collection, ""),
				data="cover=" + cover,
				headers={"Content-type": "application/x-www-form-urlencoded"}
			)
		if response.status_code != 204:
			self.raise_exception(response.status_code, "cannot set cover of collection {} to {}: {}".format(
				collection, cover, response.status_code
			))

	def list_public_blobs(self, user = ""):
		response = self.m_session.get(self.__url("/query/blob_set", {"public":user, "json":""}))
		if response.status_code != 200:
			self.raise_exception(response.status_code, "cannot public blobs: {}")
		return self.__elements_to_blobs(response.json())
