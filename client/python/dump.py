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

	album_dir = os.path.join(blob_dir, album.name())
	if not os.path.isdir(album_dir):
		Path(album_dir).mkdir(mode=0o0700, parents=True, exist_ok=True)

	coll = source.get_collection(album.name())
	for blobid, blob in coll.m_blobs.items():
		downloaded_file = os.path.join(album_dir, blob.filename())

		if not os.path.isfile(downloaded_file) or os.path.getsize(downloaded_file) == 0:
			print("downloading {} to {}".format(blob.filename(), downloaded_file))
			try:
				file_download = source.get_blob(coll.name(), blob.id(), rendition="master")
				with open(downloaded_file, "wb") as output_file:
					output_file.write(file_download.data())

				if blob.perm() == "private":
					os.chmod(downloaded_file, 0o600)
				elif blob.perm() == "shared":
					os.chmod(downloaded_file, 0o640)
				elif blob.perm() == "public":
					os.chmod(downloaded_file, 0o644)

				print("downloaded {}: {} bytes".format(blob.filename(), os.path.getsize(downloaded_file)))

			except hrb.HrbException as e:
				print("cannot download file: {}".format(e))
