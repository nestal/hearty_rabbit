import requests

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

		print("user = {}".format(user))
		response = self.m_session.get(self.m_site + "/query/collection?user=" + user + "&json")
		if response.status_code != 200:
			raise ValueError("cannot query album list: {0}".format(response.status_code))

		return response.json()["colls"]
