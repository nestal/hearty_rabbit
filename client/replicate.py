#!/usr/bin/python3

import os, sys
import requests
import urllib
from io import BytesIO

source = requests.Session()
dest   = requests.Session()
dest.verify = "../../etc/hearty_rabbit/certificate.pem"

# login to source site
login_response = source.post(
	"https://www.nestal.net/login",
	data="username=" + sys.argv[1] + "&password=" + sys.argv[2],
	headers={"Content-type": "application/x-www-form-urlencoded"}
)
if login_response.status_code != 200 or source.cookies.get("id") == "":
	print("login incorrect to source HeartyRabbit")
	sys.exit(-1)

# login to local site
#login_response = dest.post(
#	"https://localhost:4433/login",
#	data="username=" + sys.argv[1] + "&password=" + sys.argv[2],
#	headers={"Content-type": "application/x-www-form-urlencoded"}
#)
#if login_response.status_code != 200 or dest.cookies.get("id") == "":
#	print "login incorrect to destination HeartyRabbit"
#	sys.exit(-1)

# get list of album from source site
# TODO: use new API
album_list = source.get("https://www.nestal.net/listcolls/" + sys.argv[1])
for album in album_list.json()["colls"].keys():
	print(album)
	album_json = source.get("https://www.nestal.net/coll/" + sys.argv[1] + "/" + urllib.parse.quote_plus(album))
	print(album_json.json()["elements"])
