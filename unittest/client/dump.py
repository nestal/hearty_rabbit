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
login_response = source.post(
	site + "/login",
	data="username=" + user + "&password=" + password,
	headers={"Content-type": "application/x-www-form-urlencoded"}
)
if login_response.status_code != 200 or source.cookies.get("id") == "":
	print("source site login incorrect: {0}".format(login_response.status_code))
	exit(-1)

album_list = source.get(site + "/query/collection?user=" + user + "&json")
if album_list.status_code != 200:
	print("cannot query album list: {0}".format(album_list.status_code))
	exit(-1)

# save album list to file
with open(user + ".json", "w") as album_list_file:
	json.dump(album_list.json()["colls"], album_list_file)

for album_name in album_list.json()["colls"].keys():

	print("downloading album: {0}".format(album_name))
	album = source.get(site + "/view/" + user + "/" + album_name + "?json")

	# save elements list of album to file
	with open(album_name + ".json", "w") as element_file:
		json.dump(album.json()["elements"], element_file)

	for blobid, coll_entry in album.json()["elements"].items():
		dest_path = Path("{0}/{1}/{2}".format(blob_dir, blobid[0:2], blobid))
		if not dest_path.is_file():
			print("downloading " + dest_path)
