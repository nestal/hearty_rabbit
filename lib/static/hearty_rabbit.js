/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

const default_album = "(default album)";

// Default permission of newly uploaded files
let default_perm    = "private";

let last_moved_album = "";

function get_blob_url(blobid, rendition)
{
	if (dir.collection != null && dir.owner != null)
		return path_url("view", blobid) + (rendition != null ? "?" + rendition : "");
	else
		return "/query/blob?id=" + blobid + (rendition != null ? "&" + rendition : "");
}

function path_url(action, filename)
{
	return path_url_with_collection(action, (dir.collection == null) ? "" : dir.collection, filename);
}

function path_url_with_collection(action, collection, filename, option)
{
	if (!dir.owner)
		return "/";

	collection = (collection.length > 0 ? "/" : "") + collection;
	option = (option && option.length > 0 ? "?"+option : "");

	return "/" + action + "/" + dir.owner + collection + "/" + filename + option;
}

function add_coll(coll)
{
	// create one new entry for each blob
	const proto = document.getElementById("hrb-entry-prototype");

	const entry = proto.cloneNode(true);
	entry.addEventListener("click", ()=>{change_dir(coll);});

	entry.id = "hrb-entry-" + coll;
	assign_image_to_entry(
		entry,
		path_url_with_collection("view", coll, dir.colls[coll].cover) + "?thumbnail",
		(coll.length > 0 ? coll : default_album)
	);
	entry.querySelector("i.perm-overlay-box").style.display = "none";

	// Show the cloned node
	document.getElementById("hrb-entry-root").insertBefore(entry, proto);

	// hide the message that says there's no images
	document.getElementById("message").style.display = "none";
}

function assign_image_to_entry(entry, image_url, description)
{
	console.log("assigning", image_url);

	let img = entry.querySelector("img");
	img.addEventListener("error", ev => {
		entry.remove();
		if (dir.elements != null)
			delete dir.elements[blob_id];
	});
	img.src = image_url;
	img.alt = description;
	entry.querySelector("span.text-overlay-box").textContent = description;
}

function add_element(blob_id)
{
	// assuming the element is already inserted to dir.elements
	const element = dir.elements[blob_id];

	// TODO: show non-image blobs
	if (element.mime !== "image/jpeg" && element.mime !== "image/png")
		return;

	// create one new entry for each blob
	const proto = document.getElementById("hrb-entry-prototype");
	const root  = document.getElementById("hrb-entry-root");

	// Clone the prototype node and update its ID
	const entry = proto.cloneNode(true);
	entry.addEventListener("click", ()=>{show_image(blob_id, true);}, false);
	entry.id = "hrb-entry-" + blob_id;

	// Assign blob URL to the image in the cloned node
	assign_image_to_entry(entry, get_blob_url(blob_id, "thumbnail"), element.filename);

	// Show the cloned node
	root.insertBefore(entry, proto);

	// update perm icon after inserting the entry node to the document
	// otherwise update_perm_icon() can't find it
	update_permission_icons(blob_id);

	// hide the message "there are no image here yet" because we just added one
	document.getElementById("message").style.display = "none";
}

function upload_file(file)
{
	console.log("uploading", file.type, "file");

	let req = new XMLHttpRequest();
	req.open("PUT", path_url("upload", file.name));
	req.onload = () =>
	{
		// extract blob_id from the location
		if (req.status === 201)
		{
			const blob_id = req.getResponseHeader("location").substr(-40);
			dir.elements[blob_id] = {mime: file.type, filename: file.name, perm: "private"};

			show_image(blob_id, true);
			add_element(blob_id);

			// automatically change permission
			if (dir.elements[blob_id].perm !== default_perm)
				set_blob_permission(blob_id, default_perm);
		}
		else
		{
			console.warn("Upload failure:", req.status);
			alert("onload(): cannot upload file: " + req.statusText);
			close_overlays(false);
		}
	};
	req.onerror = () =>
	{
		alert("onerror(): cannot upload file: " + req.statusText);
		close_overlays(false);
	};
	req.send(file);
	show_lightbox("/lib/loading.svg");
}

function show_dialog(dialog_id)
{
	document.getElementById(dialog_id).style.display = "flex";
	document.body.style.overflow = "hidden";
}

function main()
{
	console.assert(window.dir);

	// Generic handling of popstate and direct jump from location bar
	window.onpopstate = ev =>
	{
		console.log("poping to = ", document.location);
		if (ev.state !== null)
			window.dir = ev.state;
		list_dir();
	};
	console.log("replace state", document.location.href);
	history.replaceState(dir, "", document.location.href);
	list_dir();

	// add all event handlers
	document.body.addEventListener("paste", paste);
	document.getElementById("light-box-image").addEventListener("error", ev =>
	{
		document.getElementById("light-box-image").src = "/lib/rabbit_head.svg";
	});
	document.getElementById("file-upload").addEventListener("change", function()
	{
		for (let i = 0; i < this.files.length; i++)
			upload_file(this.files[i]);
	});
	document.addEventListener("dragenter", ev=>{ev.preventDefault();}, false);
	document.addEventListener("dragover",  ev=>{ev.preventDefault();}, false);
	document.addEventListener("drop", event =>
	{
		event.preventDefault();
		let files = event.target.files || event.dataTransfer.files;

		// process all File objects
		for (let i = 0, file; file = files[i]; i++)
			upload_file(file);
	}, false);
	document.getElementById("login-nav").addEventListener("click", ev =>
	{
		ev.preventDefault();
		show_dialog("login-dialog");
	}, false);
	document.getElementById("create-album-nav").addEventListener("click", ev =>
	{
		ev.preventDefault();
		show_dialog("create-album-dialog");
	}, false);
	document.getElementById("delete-image").addEventListener("click", delete_image, false);
	document.getElementById("make-image-private").addEventListener("click", ev =>
	{
		ev.preventDefault();
		set_blob_permission(document.getElementById("light-box").dataset.blob_id, "private");
	}, false);
	document.getElementById("share-image").addEventListener("click", ev =>
	{
		ev.preventDefault();
		set_blob_permission(document.getElementById("light-box").dataset.blob_id, "shared");
	}, false);
	document.getElementById("make-image-public").addEventListener("click", ev =>
	{
		ev.preventDefault();
		set_blob_permission(document.getElementById("light-box").dataset.blob_id, "public");
	}, false);
	document.getElementById("move-image").addEventListener("click", ev =>
	{
		ev.preventDefault();

		// save blob ID before closing light-box
		document.getElementById("move-blob-dialog").dataset.blob_id =
			document.getElementById("light-box").dataset.blob_id;
		close_overlays(false);

		load_album_list();

		// fill the album with the last album the user just moved to
		if (window.last_moved_album)
			document.getElementById("dest-album-name").value = window.last_moved_album;

		show_dialog("move-blob-dialog");
	}, false);
	document.getElementById("create-album-btn").addEventListener("click", ev =>
	{
		const name = document.getElementById("new-album-name").value;
		console.log("creating album with name", name);
		window.location = path_url_with_collection("view", name, "");
	}, false);
	document.getElementById("move-blob-btn").addEventListener("click", ev =>
	{
		const blob_id = document.getElementById("move-blob-dialog").dataset.blob_id;
		const dest    = document.getElementById("dest-album-name").value;
		post_blob(blob_id, "move="+dest,
			() =>
			{
				// display the image in its new collection
				console.log("move success!");
				close_overlays(false);
				change_dir(dest, ()=>{show_image(blob_id, true);});
				window.last_moved_album = dest;
			}
		);
	});
	let close_btns = document.querySelectorAll(".close-overlays");
	for (let i = 0; i < close_btns.length; i++)
		close_btns[i].addEventListener("click", ()=>{close_overlays(true);}, false);
}

function list_dir()
{
	console.log("dispatching according to location:", document.location.pathname);

	// clear all hrb-entries and load the new ones from window.dir
	const root = document.getElementById("hrb-entry-root");
	let entries = root.querySelectorAll("div.hrb-entry");
	for (let i = 0; i < entries.length; i++)
	{
		if (entries[i] !== document.getElementById("hrb-entry-prototype"))
			entries[i].remove();
	}
	document.getElementById("message").textContent = "There are no image here yet.";
	document.getElementById("message").style.display = "block";

	if (dir.elements)
	{
		if (dir.collection != null)
			document.getElementById("headline").textContent = "Album: " + (dir.collection.length > 0 ? dir.collection : default_album);

		// Load images
		Object.keys(dir.elements).forEach(blob_id =>
		{
			add_element(blob_id);
		});
	}
	else if (dir.colls)
	{
		if (dir.username != null && dir.username.length > 0)
			document.getElementById("headline").textContent = "List of " + dir.username + "'s Albums";

		Object.keys(dir.colls).forEach(colls =>
		{
			add_coll(colls);
		});
	}

	// User logged in
	if (dir.username && dir.username.length > 0)
	{
		// Display the user's name
		document.getElementById("show-username").textContent = dir.username;

		let to_show = document.querySelectorAll(".topnav .show-only-after-logged-in");
		for (let i = 0; i < to_show.length; i++)
			to_show[i].style.display = "inline";
		let to_hide = document.querySelectorAll(".topnav .show-only-before-logged-in");
		for (let i = 0; i < to_hide.length; i++)
			to_hide[i].style.display = "none";
	}
	else
	{
		let to_hide = document.querySelectorAll(".topnav .show-only-after-logged-in");
		for (let i = 0; i < to_hide.length; i++)
			to_hide[i].style.display = "none";
		let to_show = document.querySelectorAll(".topnav .show-only-before-logged-in");
		for (let i = 0; i < to_show.length; i++)
			to_show[i].style.display = "inline";

		document.getElementById("login-from").value = encodeURI(window.location.pathname);

		// login incorrect?
		if (dir.login_message)
		{
			document.getElementById("login-dialog").style.display = "flex";
			document.getElementById("login-message").textContent = dir.login_message;
		}
	}
	if (dir.error_message)
	{
		document.getElementById("message").style.display = "block";
		document.getElementById("message").textContent = dir.error_message;
	}

	if (document.location.hash.length === 0 && !dir.login_message)
	{
		close_overlays(false);
	}
	else if (document.location.hash.length === 41)
	{
		show_image(window.location.hash.substr(1,40), false);
	}
}

function change_dir(collection, onsuccess)
{
	let req = new XMLHttpRequest();
	req.open("GET", path_url_with_collection("view", collection, "", "json"));
	req.onload = () =>
	{
		// extract blob_id from the location
		if (req.status === 200)
		{
			console.log("pushing state", dir);

			let json = JSON.parse(req.responseText);
			console.log("get dir list", json);

			window.dir = json;
			list_dir();
			history.pushState(window.dir, "", path_url_with_collection("view", collection, ""));

			if (onsuccess)
				onsuccess();
		}
		else
		{
		}
	};
	req.send();
}

function close_overlays(push_state)
{
	if (push_state)
	{
		let url = new URL(document.location.href);
		url.hash = "";
		console.log("pushing state", url.href);
		history.pushState(window.dir, "", url.href);
	}

	document.getElementById("login-dialog").style.display = "none";
	document.getElementById("create-album-dialog").style.display = "none";
	document.getElementById("light-box").style.display = "none";
	document.getElementById("move-blob-dialog").style.display = "none";
	delete document.getElementById("light-box").dataset.blob_id;

	document.getElementById("light-box-image").src = "/lib/loading.svg";

	// restore the scrollbar
	document.body.style.overflow = "auto";
}

function show_image(blob_id, push_state)
{
	if (push_state)
		history.pushState(window.dir, "", document.location + "#" + blob_id);

	// hide edit buttons if we don't know their collection
	// because we don't know the URL to POST
	const hide_btns = [
		"make-image-private", "share-image", "make-image-public", "move-image",
		"delete-image"
	];
	hide_btns.forEach(btn => {
		document.getElementById(btn).style.display = (dir.collection == null ? "none" : "inline");
	});

	show_lightbox(get_blob_url(blob_id));
	document.getElementById("light-box").dataset.blob_id = blob_id;
	update_permission_buttons(blob_id);
}

function update_permission_icons(blob_id)
{
	// update perm icon in hrb-entry
	const entry = document.getElementById("hrb-entry-" + blob_id);
	const element = dir.elements[blob_id];

	const perm_icon = entry.querySelector("i.perm-overlay-box.material-icons");
	if (element.perm === "private")
		perm_icon.textContent = "person";
	else if (element.perm === "public")
		perm_icon.textContent = "public";
	else if (element.perm === "shared")
		perm_icon.textContent = "people";
}

function update_permission_buttons(blob_id)
{
	const element = dir.elements[blob_id];

	// update permission buttons in lightbox
	if (element.perm === "private")
	{
		document.getElementById("make-image-private").classList.add("highlighted");
		document.getElementById("share-image")       .classList.remove("highlighted");
		document.getElementById("make-image-public") .classList.remove("highlighted");
	}
	else if (element.perm === "shared")
	{
		document.getElementById("make-image-private").classList.remove("highlighted");
		document.getElementById("share-image")       .classList.add("highlighted");
		document.getElementById("make-image-public") .classList.remove("highlighted");
	}
	else if (element.perm === "public")
	{
		document.getElementById("make-image-private").classList.remove("highlighted");
		document.getElementById("share-image")       .classList.remove("highlighted");
		document.getElementById("make-image-public") .classList.add("highlighted");
	}
}

function show_lightbox(url)
{
	// unhide the lightbox and set its src to the blob URL
	document.getElementById("light-box-image").src = url;
	show_dialog("light-box");
}

function delete_image()
{
	if (confirm("Do you want to delete this image"))
	{
		const blob_id = document.getElementById("light-box").dataset.blob_id;
		console.log("deleting blob", blob_id);

		let req = new XMLHttpRequest();
		req.open("DELETE", path_url("view", blob_id));
		req.onload = () =>
		{
			// extract blob_id from the location
			if (req.status === 204)
			{
				console.log("deleted blob", blob_id, "successfully");
				delete dir.elements[blob_id];
				close_overlays(false);

				document.getElementById("hrb-entry-" + blob_id).remove();
			}
			else
			{
				console.warn("delete failure:", req.status);
			}
		};
		req.send(null);
	}
}

// Called by lightbox's next/previous buttons
function next_image(offset)
{
	const sign = (offset > 0 ? 1 : -1);

	const current = document.getElementById("light-box").dataset.blob_id;
	if (current)
	{
		const blobs = Object.keys(dir.elements);

		const index = blobs.indexOf(current);
		console.log("current index", index, blobs.length);
		for (let i = 0 ; i < blobs.length; ++i)
		{
			const next_index = (index + offset + sign*i + blobs.length) % blobs.length;
			console.log("next index", next_index, blobs.length);
			const next = blobs[next_index];
			if (dir.elements[next].mime === "image/jpeg")
			{
				show_image(next, false);
				break;
			}
		}
	}
}

function paste(e)
{
	for (let i = 0 ; i < e.clipboardData.items.length ; i++)
	{
		let item = e.clipboardData.items[i];
		console.log("Item: ", item.type);

		if (item.type.indexOf("image") !== -1)
			upload_file(item.getAsFile());
    }
}

function set_blob_permission(blobid, perm)
{
	post_blob(blobid, "perm="+perm, () =>
	{
		console.log("set permission successfully");
		dir.elements[blobid].perm = perm;
		update_permission_buttons(blobid);
		update_permission_icons(blobid);

		// by default, use the same permission for next uploaded message
		default_perm = perm;
	});
}

function post_blob(blobid, data, onsuccess)
{
	let req = new XMLHttpRequest();
	req.open("POST", path_url("view", blobid));
	req.onload = () =>
	{
		// extract blob_id from the location
		if (req.status === 204)
		{
			onsuccess();
		}
		else
		{
			console.log("cannot post blob", req.status, req.responseText);
		}
	};
	req.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
	req.send(data);
}

function load_album_list()
{
	let req = new XMLHttpRequest();
	req.open("GET", "/query/collection?user=" + dir.username + "?json");
	req.onload = () =>
	{
		// extract blob_id from the location
		if (req.status === 200)
		{
			let json = JSON.parse(req.responseText);
			Object.keys(json.colls).forEach(coll =>
			{
				let option = document.createElement("option");
				option.value = coll;
				document.getElementById("album-list").appendChild(option);
			});
		}
		else
		{
			console.warn("cannot get collection list. logged out?", req.statusText);
		}
	};
	req.send();
}
