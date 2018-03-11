/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

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
	const root = document.getElementById("hrb-entry-root");

	// Clone the prototype node and update its ID
	const entry = proto.cloneNode(true);
	entry.id = "hrb-entry-" + blob_id;

	// Assign blob URL to the image in the cloned node
	const img = entry.querySelector("img");
	img.src = "/blob/" + dir.name + "/" + blob_id;
	img.dataset.blob_id = blob_id;

	// Show the cloned node
	root.insertBefore(entry, proto);
}

function upload_file(file)
{
	console.log("uploading", file.name, file.type, "file");

	let req = new XMLHttpRequest();
	req.open("PUT", "/upload/" + dir.name + dir.path + file.name);
	req.onload = () =>
	{
		// extract blob_id from the location
		const re = /\/blob\/.*\/([a-f0-9]{40})/.exec(req.getResponseHeader("location"));
		if (re && req.status === 201)
		{
			const blob_id = re[1];
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

function on_home_page_loaded()
{
	// Display the user's name
	document.getElementById("username").textContent = dir.name;

	document.body.addEventListener("paste", paste);

	console.log("dir.elements", dir.elements);

	Object.keys(dir.elements).forEach(blob_id =>
	{
		add_blob(blob_id);
	});

	document.addEventListener("drop", event =>
	{
		event.preventDefault();
		let files = event.target.files || event.dataTransfer.files;

		// process all File objects
		for (let i = 0, file; file = files[i]; i++)
			upload_file(file);
	}, false);

	document.addEventListener("dragenter", ev=>{ev.preventDefault();}, false);
	document.addEventListener("dragover",  ev=>{ev.preventDefault();}, false);
}

function show_image(blob_id)
{
	show_lightbox("/blob/" + dir.name + "/" + blob_id);
	document.getElementById("light-box").dataset.blob_id = blob_id;
}

function show_lightbox(url)
{
	// unhide the lightbox and set its src to the blob URL
	document.getElementById("light-box-image").src = url;
	document.getElementById("light-box").style.display = "block";
}

function hide_image()
{
	document.getElementById("light-box").style.display = "none";
	delete document.getElementById("light-box").dataset.blob_id;
}

function delete_image()
{
	if (confirm("Do you want to delete this image"))
	{
		const blob_id = document.getElementById("light-box").dataset.blob_id;
		console.log("deleting blob", blob_id);

		let req = new XMLHttpRequest();
		req.open("GET", "/unlink/" + dir.name + "/" + blob_id);
		req.onload = () =>
		{
			// extract blob_id from the location
			if (req.status === 202)
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