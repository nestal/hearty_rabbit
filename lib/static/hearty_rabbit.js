/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

const default_album = "(default album)";

function path_url(action, filename)
{
	return path_url_with_collection(action, dir.collection, filename);
}

function path_url_with_collection(action, collection, filename)
{
	return "/" + action + "/" + dir.owner + (collection.length > 0 ? "/" : "") + collection + "/" + filename;
}

function add_coll(coll)
{
	console.log("adding collection", coll);

	// create one new entry for each blob
	const proto = document.getElementById("hrb-entry-prototype");

	const entry = proto.cloneNode(true);
	entry.addEventListener("click", ()=>{window.location = path_url_with_collection("view", coll, "");});
	entry.id = "hrb-entry-" + coll;
	entry.querySelector("img").src = path_url_with_collection("blob", coll, dir.colls[coll].cover);

	entry.querySelector("span.text-overlay-box").textContent = (coll.length > 0 ? coll : default_album);

	// Show the cloned node
	document.getElementById("hrb-entry-root").insertBefore(entry, proto);
}

function add_blob(blob_id)
{
	console.log("adding blob", blob_id);

	// assuming the element is already inserted to dir.elements
	const element = dir.elements[blob_id];

	// TODO: show non-image blobs
	if (element.mime !== "image/jpeg" && element.mime !== "image/png")
	{
		console.log("ignoring blob", blob_id, element.mime);
		return;
	}

	// create one new entry for each blob
	const proto = document.getElementById("hrb-entry-prototype");
	const root  = document.getElementById("hrb-entry-root");

	// Clone the prototype node and update its ID
	const entry = proto.cloneNode(true);
	entry.addEventListener("click", ()=>{show_image(blob_id);}, false);
	entry.id = "hrb-entry-" + blob_id;

	// Assign blob URL to the image in the cloned node
	entry.querySelector("img").src = path_url("blob", blob_id);
	entry.querySelector("span.text-overlay-box").textContent = element.filename;

	// Show the cloned node
	root.insertBefore(entry, proto);

	// update perm icon after inserting the entry node to the document
	// otherwise update_perm_icon() can't find it
	update_permission_icons(blob_id);
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

			console.log("uploaded blob", blob_id);
			console.log("blobs: ", dir.elements);

			show_image(blob_id);
			add_blob(blob_id);
		}
		else
		{
			console.warn("Upload failure:", req.status);
			alert("onload(): cannot upload file: " + req.statusText);
			hide_image();
		}
	};
	req.onerror = () =>
	{
		alert("onerror(): cannot upload file: " + req.statusText);
		hide_image();
	};
	req.send(file);
	show_lightbox("/loading.svg");
}

function main()
{
	// add all event handlers
	document.body.addEventListener("paste", paste);
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
		document.getElementById("login-dialog").style.display = "flex";
	}, false);
	document.getElementById("create-album-nav").addEventListener("click", ev =>
	{
		ev.preventDefault();
		document.getElementById("create-album-dialog").style.display = "flex";
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
	let close_btns = document.querySelectorAll(".close-overlays");
	for (let i = 0; i < close_btns.length; i++)
		close_btns[i].addEventListener("click", close_overlays, false);

	if (window.dir && dir.elements)
	{
		document.getElementById("headline").textContent = "Album: " + (dir.collection.length > 0 ? dir.collection : default_album);

		// Load images
		Object.keys(dir.elements).forEach(blob_id =>
		{
			add_blob(blob_id);
		});
	}
	else if (window.dir && dir.colls)
	{
		if (dir.username.length > 0)
			document.getElementById("headline").textContent = "List of " + dir.username + "'s Albums";

		Object.keys(dir.colls).forEach(colls =>
		{
			add_coll(colls);
		});
	}

	// User logged in
	if (window.dir && dir.username.length > 0)
	{
		// Display the user's name
		document.getElementById("show-username").textContent = dir.username;

		let to_show = document.querySelectorAll(".topnav .show-only-after-logged-in");
		for (let i = 0; i < to_show.length; i++)
			to_show[i].style.display = "inline-block";
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
			to_show[i].style.display = "inline-block";

		document.getElementById("login-from").value = encodeURI(window.location.pathname);

		// login incorrect?
		if (window.login_message)
		{
			document.getElementById("login-dialog").style.display = "flex";
			document.getElementById("login-message").textContent = window.login_message;
		}
	}
}

function close_overlays()
{
	document.getElementById("login-dialog").style.display = "none";
	document.getElementById("create-album-dialog").style.display = "none";
	document.getElementById("light-box").style.display = "none";
	delete document.getElementById("light-box").dataset.blob_id;
}

function show_image(blob_id)
{
	show_lightbox(path_url("blob", blob_id));
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
	document.getElementById("light-box").style.display = "flex";
}

function delete_image()
{
	if (confirm("Do you want to delete this image"))
	{
		const blob_id = document.getElementById("light-box").dataset.blob_id;
		console.log("deleting blob", blob_id);

		let req = new XMLHttpRequest();
		req.open("DELETE", path_url("blob", blob_id));
		req.onload = () =>
		{
			// extract blob_id from the location
			if (req.status === 204)
			{
				console.log("deleted blob", blob_id, "successfully");
				delete dir.elements[blob_id];
				hide_image();

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
				show_image(next);
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
	console.log("sending", JSON.stringify({perm: perm}));

	let req = new XMLHttpRequest();
	req.open("POST", path_url("blob", blobid));
	req.onload = () =>
	{
		// extract blob_id from the location
		if (req.status === 204)
		{
			console.log("set permission successfully");
			dir.elements[blobid].perm = perm;
			update_permission_buttons(blobid);
		}
		else
		{
			console.log("cannot set permission", req.status, req.responseText);
		}
	};
	req.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
	req.send("perm="+perm);
}

function new_album()
{
	const name = document.getElementById("new-album-name").value;
	console.log("creating album with name", name);

	window.location = path_url_with_collection("view", name, "");
}
