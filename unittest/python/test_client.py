#!/usr/bin/python3

import importlib.util
spec = importlib.util.spec_from_file_location("hrb", "../../client/python/HeartyRabbit.py")
hrb = importlib.util.module_from_spec(spec)
spec.loader.exec_module(hrb)

subject=hrb.Session("localhost:4433")
subject.login("sumsum", "bearbear")

print(subject.list_collections())
