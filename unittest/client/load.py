#!/usr/bin/python3
import os, sys
import requests
import json
import urllib.parse
from stat import *

# TODO: read password from stdin to avoid being saved to command history
site = "https://" + sys.argv[1]
blob_dir = sys.argv[2]
user = sys.argv[3]
password = sys.argv[4]

# connect to the destination site
dest = requests.Session()
if site == "https://localhost:4433":
	dest.verify = "../../etc/hearty_rabbit/certificate.pem"

login_response = dest.post(
	site + "/login",
	data="username=" + user + "&password=" + password,
	headers={"Content-type": "application/x-www-form-urlencoded"}
)
if login_response.status_code != 200 or dest.cookies.get("id") == "":
	print("source site login incorrect: {0}".format(login_response.status_code))
	exit(-1)

for root, dirs, files in os.walk(blob_dir):
	for file in files:
		local_path = os.path.join(root, file)

		upload_url = site + "/upload/" + user + local_path[len(blob_dir):]
		print(upload_url)
		with open(local_path, "rb") as file_src:
			upload_response = dest.put(upload_url, data=file_src)

			permission = "private"
			stat_info = os.stat(local_path)
			if S_IROTH & S_IMODE(stat_info.st_mode):
				permission = "public"
			elif S_IRGRP & S_IMODE(stat_info.st_mode):
				permission = "shared"

			print(file + " is " + permission + " " + oct(S_IMODE(stat_info.st_mode)) + " " + oct(S_IROTH))

			dest.post(
				site + upload_response.headers["Location"],
				data="perm=" + permission,
				headers={"Content-type": "application/x-www-form-urlencoded"}
			)
