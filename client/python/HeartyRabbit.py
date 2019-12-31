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
	m_site = "localhost:4433"
	m_user = ""
	m_session = requests.Session()

	def __init__(self, site):
		self.m_site = "https://" + site
		if site == "localhost:4433":
			self.m_session.verify = "../../etc/hearty_rabbit/certificate.pem"


	def login(self, user, password):
		response = self.m_session.post(
			self.m_site + "/login",
			data="username=" + user + "&password=" + password,
			headers={"Content-type": "application/x-www-form-urlencoded"}
		)

		if response.status_code != 204 or self.m_session.cookies.get("id") == "":
			raise ValueError("source site login incorrect: {0}".format(response.status_code))

		self.m_user = user


	def list_collections(self, user = None):
		if user is None:
			user = self.m_user

		response = self.m_session.get(self.m_site + "/query/collection?user=" + user + "&json")
		if response.status_code != 200:
			raise ValueError("cannot query album list: {0}".format(response.status_code))

		result = []
		for coll in response.json()["colls"]:
			result.append(Collection(coll))

		return result

	def list_blobs(self, collection, user = None):
		if user is None:
			user = self.m_user

		response = self.m_session.get(self.m_site + "/api/" + user + "/" + urllib.parse.quote_plus(collection.name()))
		if response.status_code != 200:
			raise ValueError("cannot get blobs from collections \"{}\": {}".format(
				collection.name(),
				response.status_code)
			)

		blobs = []
		for id, blob in response.json()["elements"].items():
			blobs.append(Blob(id, blob))
		return blobs;