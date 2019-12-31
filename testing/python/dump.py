#!/usr/bin/python3
import os, sys
import requests
import json
import urllib.parse
from pathlib import Path

# TODO: read password from stdin to avoid being saved to command history
site = "https://" + sys.argv[1]
blob_dir = sys.argv[2]
user = sys.argv[3]
password = sys.argv[4]

# connect to the source site
source = requests.Session()
if site == "https://localhost:4433":
	source.verify = "../../etc/hearty_rabbit/certificate.pem"

login_response = source.post(
	site + "/login",
	data="username=" + user + "&password=" + password,
	headers={"Content-type": "application/x-www-form-urlencoded"}
)
if login_response.status_code != 204 or source.cookies.get("id") == "":
	print("source site login incorrect: {0}".format(login_response.status_code))
	exit(-1)

album_list = source.get(site + "/query/collection?user=" + user + "&json")
if album_list.status_code != 200:
	print("cannot query album list: {0}".format(album_list.status_code))
	exit(-1)

for album in album_list.json()["colls"]:

	print("downloading album: {0}".format(album["coll"]))
	coll = source.get(site + "/api/" + user + "/" + urllib.parse.quote_plus(album["coll"])).json()

	album_dir = os.path.join(blob_dir, album["coll"])
	if not os.path.isdir(album_dir):
		os.mkdir(album_dir, 0o0700)

	for blobid, coll_entry in coll["elements"].items():
		source_url = site + "/view/" + user + "/" + urllib.parse.quote_plus(album["coll"]) + "/" + blobid + "?master"
		permission = coll_entry["perm"]

		downloaded_file = os.path.join(album_dir, coll_entry["filename"])
		if not os.path.isfile(downloaded_file):
			with open(downloaded_file, "wb") as output_file:
				print("downloading " + coll_entry["filename"])
				file_download = source.get(source_url, stream=True)

				for chunk in file_download.iter_content(chunk_size=1024*1024):
					if chunk:
						output_file.write(chunk)

		if permission == "private":
			os.chmod(downloaded_file, 0o600)
		elif permission == "shared":
			os.chmod(downloaded_file, 0o640)
		elif permission == "public":
			os.chmod(downloaded_file, 0o644)
