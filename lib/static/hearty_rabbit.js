/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

function path_url(action, filename)
{
	return "/" + action + "/" + dir.owner + (dir.collection.length > 0 ? "/" : "") + dir.collection + "/" + filename;
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
	entry.id = "hrb-entry-" + blob_id;

	// Assign blob URL to the image in the cloned node
	const img = entry.querySelector("img");
	img.src = path_url("blob", blob_id);
	img.addEventListener("click", ()=>{show_image(blob_id);}, false);

	// Show the cloned node
	root.insertBefore(entry, proto);
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
			dir.elements[blob_id] = {mime: file.type, filename: file.name};

			console.log("uploaded blob", blob_id);
			console.log("blobs: ", dir.elements);

			show_image(blob_id);
			add_blob(blob_id);
		}
		else
		{
			console.warn("Upload failure:", req.status);
		}
	};
	req.send(file);
	show_lightbox("/loading.svg");
}

function main()
{
	// add all event handlers
	document.body.addEventListener("paste", paste);
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
	document.getElementById("username-nav").addEventListener("click", list_collections, false);
	document.getElementById("login-nav").addEventListener("click", ev =>
	{
		ev.preventDefault();
		document.getElementById("login-box").style.display = "block";
	}, false);
	document.getElementById("delete-image").addEventListener("click", delete_image, false);
	document.getElementById("close-lightbox").addEventListener("click", ev =>
	{
		ev.preventDefault();
		document.getElementById("light-box").style.display = "none";
		delete document.getElementById("light-box").dataset.blob_id;
	}, false);
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

	// Load images
	Object.keys(dir.elements).forEach(blob_id =>
	{
		add_blob(blob_id);
	});

	// User logged in
	if (window.dir && dir.username.length > 0)
	{
		// Display the user's name
		document.getElementById("show-username").textContent = dir.username;
		document.getElementById("username-nav").style.display = "inline";
		document.getElementById("login-nav").style.display = "none";
		document.getElementById("logout-nav").style.display = "inline";
	}
	else
	{
		document.getElementById("username-nav").style.display = "none";
		document.getElementById("login-nav").style.display = "inline";
		document.getElementById("logout-nav").style.display = "none";

		// login incorrect?
		if (window.login_message)
		{
			document.getElementById("login-box").style.display = "block";
			document.getElementById("login-message").textContent = window.login_message;
		}
	}
}

function cancel_login()
{
	document.getElementById("login-box").style.display = "none";
}

function show_image(blob_id)
{
	show_lightbox(path_url("blob", blob_id));
	document.getElementById("light-box").dataset.blob_id = blob_id;

	console.log("permission is ", dir.elements[blob_id].perm);
	update_permission_buttons(blob_id);
}

function update_permission_buttons(blob_id)
{
	if (dir.elements[blob_id].perm === "private")
	{
		document.getElementById("make-image-private").querySelector("i").classList.add("highlighted");
		document.getElementById("share-image")       .querySelector("i").classList.remove("highlighted");
		document.getElementById("make-image-public") .querySelector("i").classList.remove("highlighted");
	}
	else if (dir.elements[blob_id].perm === "shared")
	{
		document.getElementById("make-image-private").querySelector("i").classList.remove("highlighted");
		document.getElementById("share-image")       .querySelector("i").classList.add("highlighted");
		document.getElementById("make-image-public") .querySelector("i").classList.remove("highlighted");
	}
	else if (dir.elements[blob_id].perm === "public")
	{
		document.getElementById("make-image-private").querySelector("i").classList.remove("highlighted");
		document.getElementById("share-image")       .querySelector("i").classList.remove("highlighted");
		document.getElementById("make-image-public") .querySelector("i").classList.add("highlighted");
	}
}

function show_lightbox(url)
{
	// unhide the lightbox and set its src to the blob URL
	document.getElementById("light-box-image").src = url;
	document.getElementById("light-box").style.display = "block";
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
				console.warn("Upload failure:", req.status);
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

function list_collections()
{
	console.log("listing collections");
}
