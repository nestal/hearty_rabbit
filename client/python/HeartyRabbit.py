import requests
import urllib.parse

class Blob:
	m_json = None
	m_id = None

	def __init__(self, id, json):
		self.m_id = id
		self.m_json = json

	def filename(self):
		return self.m_json["filename"]

	def id(self):
		return self.m_id

class Collection:
	m_json = None

	def __init__(self, json):
		self.m_json = json

	def name(self):
		return self.m_json["coll"]


class Session:
	m_site = None
	m_user = ""
	m_session = None

	def __init__(self, site = "localhost:4433", cert = None):
		self.m_session = requests.Session()
		self.m_site = urllib.parse.urlparse("https://" + site)

		# This is a hack for testing purpose.
		if self.m_site.netloc == "localhost:4433" and cert is None:
			self.m_session.verify = "../../etc/hearty_rabbit/certificate.pem"

	def url(self, path, query = None):
		url = self.m_site._replace(path = path)
		if query is not None:
			url = url._replace(query = urllib.parse.urlencode(query))

		return urllib.parse.urlunparse(url)

	@staticmethod
	def raise_exception(status_code, format_string):
		message = format_string.format(status_code)
		if status_code == 404:
			raise FileNotFoundError(message)
		elif status_code == 403:
			raise PermissionError(message)
		else:
			raise Exception(message)

	def login(self, user, password):
		response = self.m_session.post(
			self.url("/login"),
			data="username=" + user + "&password=" + password,
			headers={"Content-type": "application/x-www-form-urlencoded"}
		)

		if response.status_code != 204 or self.m_session.cookies.get("id") == "":
			self.raise_exception(response.status_code, "source site login incorrect: {0}")

		self.m_user = user

	def close(self):
		self.m_session.close()

	def list_collections(self, user = None):
		if user is None:
			user = self.m_user

		response = self.m_session.get(self.url("/query/collection", {"user": user}))
		if response.status_code != 200:
			self.raise_exception(response.status_code, "cannot query album list: {0}")

		result = []
		for coll in response.json()["colls"]:
			result.append(Collection(coll))

		return result

	def list_blobs(self, collection, user = None):
		if user is None:
			user = self.m_user

		response = self.m_session.get(self.url("/api/" + user + "/" + urllib.parse.quote_plus(collection)))
		if response.status_code != 200:
			self.raise_exception(response.status_code, "cannot get blobs from collections \"{}\": {}".format(
				collection,
				response.status_code
			))

		blobs = []
		for id, blob in response.json()["elements"].items():
			blobs.append(Blob(id, blob))
		return blobs

	def upload(self, collection, filename, data):
		response = self.m_session.put(
			self.url("/upload/{}/{}/{}".format(self.m_user, collection, filename)),
			data=data
		)
		if response.status_code != 201:
			self.raise_exception(response.status_code, "cannot upload blobs to collections \"{}\": {}".format(
				collection,
				response.status_code
			))
		return response.headers["Location"][-40:]

	def get_blob(self, collection, id, user = None, rendition = ""):
		if user is None:
			user = self.m_user

		query = {}
		if rendition != "":
			query["rendition"] = rendition

		response = self.m_session.get(self.url("/api/{}/{}/{}".format(user, collection, id), query))
		if response.status_code != 200:
			self.raise_exception(response.status_code, "cannot get blob {} from user \"{}\": {}".format(
				id, user, response.status_code
			))

		return {"id":id, "mime":response.headers["Content-type"], "data":response.content}

	def query_blob(self, blob, rendition = ""):
		query = {"id": blob}
		if rendition != "":
			query["rendition"] = rendition
		response = self.m_session.get(self.url("/query/blob", query))
		if response.status_code != 200:
			self.raise_exception(response.status_code, "cannot query blob {}: {}".format(
				blob, response.status_code
			))
		return {"id":blob, "mime":response.headers["Content-type"], "data":response.content}
