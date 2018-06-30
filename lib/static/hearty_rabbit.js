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
let toast_timer = null;

function make_query_string(map)
{
	let result = "";
	if (map != null)
		map.forEach((value, key) =>
		{
			result += "&" + key + (value.length > 0 ? '=' + value : "");
		});
	return result.length > 0 ? "?" + result.substr(1) : "";
}

function get_blob_url(blobid, rendition)
{
	const element = dir.elements ? dir.elements[blobid] : null;
	const coll  = element != null && element.collection != null ? element.collection : dir.collection;
	const owner = element != null && element.owner      != null ? element.owner : dir.username;

	let qmap = new Map();
	if (rendition != null)
		qmap.set("rendition", rendition);
	if (dir.auth != null)
		qmap.set("auth", dir.auth);

	if (coll != null && owner != null)
		return "/api/" + owner + "/" + coll + "/" + blobid + make_query_string(qmap);
	else
		return "/query/blob?id=" + blobid + make_query_string(qmap);
}

function path_url(action, fields)
{
	let owner = fields.owner;
	if (owner == null)
		owner = dir.username;
	if (owner == null)
		return "/";

	let collection = fields.collection;
	if (collection == null)
		collection = dir.collection;

	let query = fields.query;
	if (query == null && dir.auth != null)
		query = new Map();
	if (dir.auth != null)
		query.set("auth", dir.auth);

	let filename = fields.filename;
	if (filename == null)
		filename = "";

	return "/" + action + "/" + owner + (collection.length > 0 ? "/" : "") + collection + "/" + filename +
		make_query_string(query);
}

function add_coll(coll)
{
	const path_intent = {
		owner:coll.owner,
		collection:coll.coll,
		filename:coll.cover,
		query:new Map([["rendition", "thumbnail"]])
	};
	console.log("adding coll", path_url("api", path_intent));

	// create one new entry for each blob
	const proto = document.getElementById("hrb-entry-prototype");

	const entry = proto.cloneNode(true);
	entry.addEventListener("click", ()=>{change_dir(path_intent);});

	entry.id = "hrb-entry-" + coll.coll;
	assign_image_to_entry(
		entry,
		path_url("api", path_intent),
		(coll.coll.length > 0 ? coll.coll : default_album)
	);
	entry.querySelector("i.perm-overlay-box").style.display = "none";

	// Show the cloned node
	document.getElementById("hrb-entry-root").insertBefore(entry, proto);

	// hide the message that says there's no images
	document.getElementById("message").style.display = "none";
}

function assign_image_to_entry(entry, image_url, description)
{
	let img = entry.querySelector("img");
	img.addEventListener("error", ev =>
	{
		const blob_id = entry.id.replace(/^hrb-entry-/, '');
		console.warn("error loading image", image_url, blob_id);
		img.src = "/lib/rabbit_head.svg";
	});
	img.src = image_url;
	img.alt = description;
	entry.querySelector("span.text-overlay-box").textContent = description;
}

function is_image(mime)
{
	return mime.indexOf("image/") !== -1;
}

function add_element(blob_id)
{
	// assuming the element is already inserted to dir.elements
	const element = dir.elements[blob_id];

	// TODO: show non-image blobs
	if (!is_image(element.mime))
		return;

	// create one new entry for each blob
	const proto = document.getElementById("hrb-entry-prototype");
	const root  = document.getElementById("hrb-entry-root");

	// Clone the prototype node and update its ID
	const entry = proto.cloneNode(true);
	entry.id = "hrb-entry-" + blob_id;
    entry.addEventListener("click", ev=>
	{
		ev.preventDefault();
		if (get_selected_blobs().length === 0)
			show_image(blob_id, true);
		else
			toggle_select_blob(blob_id);
	}, false);

	// Assign blob URL to the image in the cloned node
	assign_image_to_entry(entry, get_blob_url(blob_id, "thumbnail"), element.filename);

	// Show the cloned node
	root.insertBefore(entry, proto);

	// update perm icon after inserting the entry node to the document
	// otherwise update_perm_icon() can't find it
	update_permission_icons(blob_id);

	// select the image if the user clicks on the perm icon
    entry.querySelector(".perm-overlay-box").addEventListener("click", ev=>
	{
		toggle_select_blob(blob_id);
		ev.stopPropagation();
	}, false);

	// hide the message "there are no image here yet" because we just added one
	document.getElementById("message").style.display = "none";
}

function show_dialog(dialog_id)
{
	document.getElementById(dialog_id).style.display = "flex";
	document.body.style.overflow = "hidden";
}

function show_move_dialog()
{
	update_album_list();

	// fill the album with the last album the user just moved to
	if (window.last_moved_album)
		document.getElementById("dest-album-name").value = window.last_moved_album;

	show_dialog("move-blob-dialog");
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
	document.getElementById("headline").addEventListener("click", ev =>
	{
		document.getElementById("headline").setAttribute("contenteditable", true);
	}, false);
	document.getElementById("headline").addEventListener("blur", ev =>
	{
		document.getElementById("headline").setAttribute("contenteditable", false);
		console.log("headline = ", document.getElementById("headline").innerText);
	}, false);
	document.getElementById("light-box-image").addEventListener("error", ev =>
	{
		console.log("light-box-image error", ev.target);
		document.getElementById("light-box-image").src = "/lib/rabbit_head.svg";
	}, false);
	document.getElementById("username-nav").addEventListener("click", ev =>
	{
		change_dir();
	}, false);
	document.getElementById("logout-nav").addEventListener("click", ev =>
	{
		ev.preventDefault();
		let req = new XMLHttpRequest();
		req.overrideMimeType("text/plain");
		req.open("GET", "/logout");
		req.onload = () =>
		{
			if (req.status === 200)
			{
				console.log("logout successful");
				delete dir.username;
				change_dir({owner:dir.username, collection:dir.collection});
			}
			else
			{
				console.log("cannot logout", req.status, req.responseText);
			}
		};
		req.send();
	});
	document.getElementById("file-upload").addEventListener("change", function()
	{
		for (let i = 0; i < this.files.length; i++)
			queue_action({upload: this.files[i]});
	});
	document.addEventListener("dragenter", ev=>{ev.preventDefault();}, false);
	document.addEventListener("dragover",  ev=>{ev.preventDefault();}, false);
	document.addEventListener("drop", event =>
	{
		event.preventDefault();
		let files = event.target.files || event.dataTransfer.files;

		console.log("dropped", files.length, "files");

		// process all File objects
		for (let i = 0, file; file = files[i]; i++)
			queue_action({upload: file});
	}, false);
	document.getElementById("login-nav").addEventListener("click", ev =>
	{
		ev.preventDefault();

		// make sure the submit button is not disabled by last login
		document.querySelector("#login-dialog button").disabled = false;
		document.getElementById("login-username").disabled = false;
		document.getElementById("login-password").disabled = false;
		document.getElementById("login-message").textContent = "Please login:";
		show_dialog("login-dialog");
	}, false);
	document.getElementById("create-album-nav").addEventListener("click", ev =>
	{
		ev.preventDefault();
		show_dialog("create-album-dialog");
	}, false);
	document.getElementById("move-image-nav").addEventListener("click", ev =>
	{
		ev.preventDefault();
		show_move_dialog();
	}, false);
	document.getElementById("make-image-private-nav").addEventListener("click", ev =>
	{
		ev.preventDefault();
		get_selected_blobs().forEach(blob => {queue_action({blob:blob, perm:"private"});});
		window.default_perm = "private";
		select_all_blobs(false);
	}, false);
	document.getElementById("make-image-shared-nav").addEventListener("click", ev =>
	{
		ev.preventDefault();
		get_selected_blobs().forEach(blob => {queue_action({blob:blob, perm:"shared"});});
		window.default_perm = "shared";
		select_all_blobs(false);
	}, false);
	document.getElementById("make-image-public-nav").addEventListener("click", ev =>
	{
		ev.preventDefault();
		get_selected_blobs().forEach(blob => {queue_action({blob:blob, perm:"public"});});
		window.default_perm = "public";
		select_all_blobs(false);
	}, false);
	document.getElementById("share-nav").addEventListener("click", ev =>
	{
		ev.preventDefault();

		post_blob("", "share=create").then((xhr) =>
		{
			console.log("share link", xhr.getResponseHeader("location"));
			show_dialog("share-link-dialog");
			document.getElementById("share-link-textbox").textContent =
				document.location.protocol + "//" + document.location.host + xhr.getResponseHeader("location");
		});
	}, false);
	document.getElementById("copy-shared-link").addEventListener("click", ev =>
	{
		console.log("copying");
		ev.preventDefault();
		let range = document.createRange();
		range.selectNodeContents(document.getElementById("share-link-textbox"));
		window.getSelection().addRange(range);
		document.execCommand("copy");
	}, false);
	document.getElementById("delete-image").addEventListener("click", delete_image, false);
	document.getElementById("make-cover").addEventListener("click", ev =>
	{
		ev.preventDefault();
		let cover = document.getElementById("light-box").dataset.blob_id;
		post_blob("", "cover=" + cover).then(() =>
		{
			console.log("set cover to", cover);
			if (dir.meta == null)
				dir.meta = {cover: cover};
			else
				dir.meta.cover = cover;
			document.getElementById("make-cover").classList.add("highlighted");
		});
	}, false);
	document.getElementById("make-image-private").addEventListener("click", ev =>
	{
		ev.preventDefault();
		set_blob_permission(document.getElementById("light-box").dataset.blob_id, "private");
	}, false);
	document.getElementById("make-image-shared").addEventListener("click", ev =>
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

		show_move_dialog();
	}, false);
	document.querySelector("#create-album-dialog form").addEventListener("submit", ev =>
	{
		ev.preventDefault();
		const name = document.getElementById("new-album-name").value;
		change_dir({collection:name, filename:""});
	}, false);
	document.querySelector("#move-blob-dialog form").addEventListener("submit", ev =>
	{
		ev.preventDefault();

		if (document.getElementById("move-blob-dialog").dataset.blob_id != null)
		{
			queue_action({
				move: document.getElementById("move-blob-dialog").dataset.blob_id,
				dest: document.getElementById("dest-album-name").value
			});
		}
		else
		{
			let selected = get_selected_blobs();
			selected.forEach(blob_id =>
			{
				queue_action({
					move: blob_id,
					dest: document.getElementById("dest-album-name").value
				});
			});
		}
	}, false);
	document.querySelector("#login-dialog form").addEventListener("submit", ev =>
	{
		ev.preventDefault();
		let req = new XMLHttpRequest();
		req.overrideMimeType("text/plain");
		req.open("POST", "/login");
		req.onload = () =>
		{
			if (req.status === 204)
			{
				dir.username = document.getElementById("login-username").value;
				console.log("user", dir.username, "login successful");

				close_overlays(false);
				change_dir({owner:dir.username, collection:dir.collection});

				// clear password text... hopefully will clear it from memory too
				document.getElementById("login-password").value = "";
			}
			else
			{
				//console.log("cannot post blob", req.status, req.responseText);
				document.getElementById("login-message").textContent = "Login incorrect! Please try again:";
			}
		};
		req.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
		req.send(
			"username=" + document.getElementById("login-username").value +
			"&password=" + document.getElementById("login-password").value
		);

		// disable submit button
		document.querySelector("#login-dialog button").disabled = true;
		document.getElementById("login-username").disabled = true;
		document.getElementById("login-password").disabled = true;
		document.getElementById("login-message").textContent = "Please wait... Logging in..";
	}, false);
	let close_btns = document.querySelectorAll(".close-overlays");
	for (let i = 0; i < close_btns.length; i++)
		close_btns[i].addEventListener("click", ()=>{close_overlays(true);}, false);
}

function sort_elements()
{
	// Sort images by their timestamps before showing them
	let blobs = Object.keys(dir.elements);
	blobs.sort((a,b) =>
	{
		return dir.elements[a].timestamp - dir.elements[b].timestamp;
	});
	return blobs;
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

		// Sort images by their timestamps before showing them
		sort_elements().forEach(add_element);
	}
	else if (dir.colls)
	{
		if (dir.username != null && dir.username.length > 0)
			document.getElementById("headline").textContent = "List of " + dir.username + "'s Albums";

		dir.colls.forEach(add_coll);
	}

	// User logged in
	if (dir.username && dir.username.length > 0)
	{
		// Display the user's name
		document.getElementById("show-username").textContent = dir.username;

		let to_show = document.querySelectorAll(".topnav .show-only-after-logged-in");
		for (let i = 0; i < to_show.length; i++)
			to_show[i].style.display = "block";
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
			to_show[i].style.display = "block";
	}
	if (dir.error_message)
	{
		document.getElementById("message").style.display = "block";
		document.getElementById("message").textContent = dir.error_message;
	}

	if (!dir.blob && !dir.login_message)
	{
		close_overlays(false);
	}
	else if (dir.blob != null && dir.blob.length === 40)
	{
		show_image(dir.blob, false);
	}

	update_selected_blobs();
}

function change_dir(input_path_intent)
{
	// change to the parent directory
	let path_intent = Object.assign({}, input_path_intent);
	delete path_intent.filename;
	delete path_intent.query;

	console.log("changing dir to ", path_intent, input_path_intent);

	return new Promise((resolve, reject)=>
	{
		let req = new XMLHttpRequest();
		req.open("GET", path_intent.collection != null ?
			path_url("api", path_intent) :
			(dir.username != null ? "/query/collection?user=" + dir.username + "&json" : "/query/blob_set?public&json")
		);

		req.onload = () =>
		{
			if (req.status === 200)
			{
				console.log("pushing state", dir);
				window.dir = JSON.parse(req.responseText);
				list_dir();
				history.pushState(
					window.dir,
					"",
					path_intent.collection != null ? path_url("view", path_intent) : "/"
				);

				resolve();
			}
			else
			{
				reject();
			}
		};
		req.onerror = () => {reject();};
		req.send();
	});
}

function close_overlays(push_state)
{
	if (push_state)
	{
		if (dir.blob != null)
			delete dir.blob;

		let url = path_url("view", {collection:dir.collection, filename:""});
		console.log("pushing state", url);
		history.pushState(window.dir, "", url);
	}

	document.getElementById("login-dialog").style.display = "none";
	document.getElementById("create-album-dialog").style.display = "none";
	document.getElementById("share-link-dialog").style.display = "none";
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
	{
		window.dir.blob = blob_id;
		history.pushState(window.dir, "", path_url("view", {collection:dir.collection, filename:blob_id}));
	}

	// hide edit buttons if we don't know their collection
	// because we don't know the URL to POST
	const hide_btns = [
		"make-image-private", "make-image-shared", "make-image-public", "move-image",
		"delete-image", "make-cover"
	];
	hide_btns.forEach(btn => {
		document.getElementById(btn).style.display = (dir.collection == null ? "none" : "inline");
	});

	// update "make-cover" button
	if (dir.meta && dir.meta.cover === blob_id)
		document.getElementById("make-cover").classList.add("highlighted");
	else
		document.getElementById("make-cover").classList.remove("highlighted");

	// show metadata
	get_image_meta({filename:blob_id}).then(meta =>
	{
		console.log("returned image meta = ", meta);
		document.getElementById("meta-filename").textContent = meta.filename;
		document.getElementById("meta-mime").textContent = meta.mime;
		document.getElementById("meta-datetime").textContent = new Date(meta.timestamp).toLocaleString();
	});

	show_lightbox(get_blob_url(blob_id));
	document.getElementById("light-box").dataset.blob_id = blob_id;
	update_permission_buttons(blob_id);
	select_all_blobs(false);
}

function update_permission_icons(blob_id)
{
	// update perm icon in hrb-entry
	const entry = document.getElementById("hrb-entry-" + blob_id);
	const element = dir.elements[blob_id];

	if (element != null)
	{
		const perm_icon = entry.querySelector("i.perm-overlay-box.material-icons");
		if (element.selected != null && element.selected === true)
			perm_icon.textContent = "done";
		else if (element.perm === "private")
			perm_icon.textContent = "person";
		else if (element.perm === "public")
			perm_icon.textContent = "public";
		else if (element.perm === "shared")
			perm_icon.textContent = "people";
	}
}

function update_permission_buttons(blob_id)
{
	const element = dir.elements[blob_id];

	// update permission buttons in lightbox
	if (element.perm === "private")
	{
		document.getElementById("make-image-private").classList.add("highlighted");
		document.getElementById("make-image-shared") .classList.remove("highlighted");
		document.getElementById("make-image-public") .classList.remove("highlighted");
	}
	else if (element.perm === "shared")
	{
		document.getElementById("make-image-private").classList.remove("highlighted");
		document.getElementById("make-image-shared") .classList.add("highlighted");
		document.getElementById("make-image-public") .classList.remove("highlighted");
	}
	else if (element.perm === "public")
	{
		document.getElementById("make-image-private").classList.remove("highlighted");
		document.getElementById("make-image-shared") .classList.remove("highlighted");
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
		req.open("DELETE", path_url("api", {filename:blob_id}));
		req.onload = () =>
		{
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
		const blobs = sort_elements();
		const index = blobs.indexOf(current);
		for (let i = 0 ; i < blobs.length; ++i)
		{
			const next_index = (index + offset + sign*i + blobs.length) % blobs.length;
			const next = blobs[next_index];
			if (is_image(dir.elements[next].mime))
			{
				show_image(next, true);
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

		if (is_image(item.type))
			queue_action({upload: item.getAsFile()});
    }
}

function set_blob_permission(blobid, perm)
{
	return post_blob(blobid, "perm="+perm).then(() =>
	{
		console.log("set permission successfully");
		dir.elements[blobid].perm = perm;
		update_permission_buttons(blobid);
		update_permission_icons(blobid);

		// by default, use the same permission for next uploaded message
		window.default_perm = perm;
	});
}

function get_image_meta(intent_fields)
{
	return new Promise((resolve, reject) =>
	{
		intent_fields.query = new Map([["json", ""]]);

		console.log("image meta URL:", path_url("api", intent_fields));

		let req = new XMLHttpRequest();
		req.open("GET", path_url("api", intent_fields));
		req.responseType = 'json';
		req.onload = () =>
		{
			// extract blob_id from the location
			if (req.status === 200)
			{
				let meta = req.response;

				// inject meta from dir.elements into the returned meta JSON
				let element = dir.elements[intent_fields.filename];
				if (element)
					meta = Object.assign(meta, element);

				resolve(meta);
			}
			else
			{
				reject(req.status);
			}
		};
		req.onerror = () =>
		{
			reject(req.status);
		};
		req.send();
	});
}

function update_album_list()
{
	let req = new XMLHttpRequest();
	req.open("GET", "/query/collection?user=" + dir.username + "&json");
	req.responseType = "json";
	req.onload = () =>
	{
		// extract blob_id from the location
		if (req.status === 200)
		{
			// remove all child node in the album-list data list
			// but when JSON.parse() does not thrown exception
			let album_list = document.getElementById("album-list");
			while (album_list.firstChild)
				album_list.removeChild(album_list.firstChild);

			// add the album to album list
			req.response.colls.forEach(coll =>
			{
				let option = document.createElement("option");
				option.value = coll.coll;
				album_list.appendChild(option);
			});
		}
		else
		{
			console.warn("cannot get collection list. logged out?", req.statusText);
		}
	};
	req.send();
}

function hide_toast(message)
{
	if (message != null)
	{
		show_toast(message);

		console.log("hiding toast in ten seconds");
		toast_timer = setTimeout(() =>
		{
			console.log("hiding toast");
			document.getElementById("toast").style.display = "none";
			toast_timer = null;
		}, 10000);
	}
	else
		document.getElementById("toast").style.display = "none";
}

function show_toast(message)
{
	console.log("show_toast(" + message + ")");
	if (toast_timer != null)
	{
		clearTimeout(toast_timer);
		toast_timer = null;
	}
	document.getElementById("toast").textContent	= message;
	document.getElementById("toast").style.display	= "block";
}
