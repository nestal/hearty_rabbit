#!/usr/bin/python3
import os, sys
import requests
import json
import urllib.parse
from stat import *

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

# TODO: read password from stdin to avoid being saved to command history
site = sys.argv[1]
blob_dir = sys.argv[2]
user = sys.argv[3]
password = sys.argv[4]

# connect to the destination site
dest = hrb.Session(site, cert)
dest.login(user, password)

for root, dirs, files in os.walk(blob_dir):
	for file in files:
		local_path = os.path.join(root, file)

		with open(local_path, "rb") as file_src:

			coll = root[len(blob_dir)+1:]
			blobid = dest.upload(coll, file, data=file_src)

			permission = "private"
			stat_info = os.stat(local_path)
			if S_IROTH & S_IMODE(stat_info.st_mode):
				permission = "public"
			elif S_IRGRP & S_IMODE(stat_info.st_mode):
				permission = "shared"

			print("uploaded {} {} blob {} to {}".format(file, permission, blobid, coll))
			dest.set_permission(coll, blobid, permission)
