#!/usr/bin/python3
import os, sys
import requests
import json
import urllib.parse

# download all images from nestal.net to local
source_site = "https://www.nestal.net"
dest_site = "https://localhost:4433"
user = sys.argv[1]

# connect to the source site
source = requests.Session()
login_response = source.post(
	source_site + "/login",
	data="username=" + user + "&password=" + sys.argv[2],
	headers={"Content-type": "application/x-www-form-urlencoded"}
)
if login_response.status_code != 200 or source.cookies.get("id") == "":
	print("source site login incorrect")
	exit(-1)

# connect to the destination site
dest = requests.Session()
dest.verify = "../../etc/hearty_rabbit/certificate.pem"
login_response = dest.post(
	dest_site + "/login",
	data="username=" + user + "&password=" + sys.argv[2],
	headers={"Content-type": "application/x-www-form-urlencoded"}
)
if login_response.status_code != 200 or source.cookies.get("id") == "":
	print("destination site login incorrect")
	exit(-1)

album_list = source.get(source_site + "/query/collection?user=" + user + "&json")
if album_list.status_code != 200:
	print("cannot query album list: {0}".format(album_list.status_code))
	exit(-1)

for album_name in album_list.json()["colls"].keys():
	print(album_name)

	album = source.get(source_site + "/view/" + user + "/" + album_name + "?json")
	print(album.json()["elements"])

	for blobid, coll_entry in album.json()["elements"].items():
		print(blobid + "-> " + coll_entry["filename"])
		source_url = source_site + "/view/" + user + "/" + urllib.parse.quote_plus(album_name) + "/" + blobid + "?master"
		dest_url = dest_site + "/upload/" + user + "/" + urllib.parse.quote_plus(album_name) + "/" + urllib.parse.quote_plus(coll_entry["filename"])
		permission = coll_entry["perm"]

		print(blobid + "-> " + permission)

		# get the image from source and upload to dest
		transfer = source.get(source_url)
		if transfer.status_code == 200:
			upload_response = dest.put(dest_url, data=transfer.content)

			# owner set permission
			dest.post(
				dest_site + upload_response.headers["Location"],
				data="perm=" + permission,
				headers={"Content-type": "application/x-www-form-urlencoded"}
			)
