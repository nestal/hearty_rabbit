#!/usr/bin/python3

import importlib.util
spec = importlib.util.spec_from_file_location("hrb", "../../client/python/HeartyRabbit.py")
hrb = importlib.util.module_from_spec(spec)
spec.loader.exec_module(hrb)

subject=hrb.Session("localhost:4433")
subject.login("sumsum", "bearbear")

for coll in subject.list_collections():
#	print("collection: {}".format(coll.m_json))

	blobs = subject.list_blobs(coll)
	for blob in blobs:
		print("blob {}: {}".format(blob.id(), blob.filename()))
