#!/usr/bin/python3
import os, sys
import requests
import json
import urllib.parse
from pathlib import Path

# Any better ways to do it?
import importlib.util
spec = importlib.util.spec_from_file_location(
	"hrb", os.path.join(os.path.dirname(os.path.abspath(__file__)), "HeartyRabbit.py")
)
hrb = importlib.util.module_from_spec(spec)
spec.loader.exec_module(hrb)

cert = None
if len(sys.argv) > 5:
	cert = sys.argv[5]
elif sys.argv[1] == "localhost:4433":
	cert = os.path.join(os.path.dirname(os.path.abspath(__file__)), "../../etc/hearty_rabbit/certificate.pem")

# connect to the source site
# TODO: read password from stdin to avoid being saved to command history
source = hrb.Session(sys.argv[1], cert)
source.login(sys.argv[3], sys.argv[4])

blob_dir = sys.argv[2]

album_list = source.list_collections()

for album in album_list:

	print("downloading album: {0}".format(album.name()))

	coll = source.get_collection(album.name())
	for blobid, blob in coll.m_blobs.items():
		print("downloading " + blob.id())
	# coll = source.get(site + "/api/" + user + "/" + urllib.parse.quote_plus(album["coll"])).json()
	#
	# album_dir = os.path.join(blob_dir, album["coll"])
	# if not os.path.isdir(album_dir):
	# 	os.mkdir(album_dir, 0o0700)
	#
	# for blobid, coll_entry in coll["elements"].items():
	# 	source_url = site + "/view/" + user + "/" + urllib.parse.quote_plus(album["coll"]) + "/" + blobid + "?master"
	# 	permission = coll_entry["perm"]
	#
	# 	downloaded_file = os.path.join(album_dir, coll_entry["filename"])
	# 	if not os.path.isfile(downloaded_file):
	# 		with open(downloaded_file, "wb") as output_file:
	# 			print("downloading " + coll_entry["filename"])
	# 			file_download = source.get(source_url, stream=True)
	#
	# 			for chunk in file_download.iter_content(chunk_size=1024*1024):
	# 				if chunk:
	# 					output_file.write(chunk)
	#
	# 	if permission == "private":
	# 		os.chmod(downloaded_file, 0o600)
	# 	elif permission == "shared":
	# 		os.chmod(downloaded_file, 0o640)
	# 	elif permission == "public":
	# 		os.chmod(downloaded_file, 0o644)
