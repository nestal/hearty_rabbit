/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

function add_blob(blob_id)
{
	// TODO: show non-image blobs
	if (dir.elements[blob_id].mime !== "image/jpeg")
		return;

	// create one new entry for each blob
	const proto = document.getElementById("hrb-entry-prototype");
	const root = document.getElementById("hrb-entry-root");

	// Clone the prototype node and update its ID
	const entry = proto.cloneNode(true);
	entry.id = "hrb-entry-" + blob_id;

	// Assign blob URL to the image in the cloned node
	const img = entry.querySelector("img");
	img.src = "/blob/" + blob_id;
	img.dataset.blob_id = blob_id;

	// Show the cloned node
	root.insertBefore(entry, proto);
}

function upload_file(file)
{
	console.log("uploading", file.name, file.type, "file");

	let req = new XMLHttpRequest();
	req.open("PUT", "/upload/" + file.name);
	req.onload = () =>
	{
		// extract blob_id from the location
		const re = /\/blob\/([a-f0-9]{40})/.exec(req.getResponseHeader("location"));
		if (re && req.status === 201)
		{
			const blob_id = re[1];
			dir.elements[blob_id] = {mime: file.type, filename: file.name};

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

	Object.keys(dir.elements).forEach(element =>
	{
		add_blob(element);
	});

	document.addEventListener("drop", event =>
	{
		event.preventDefault();
		let files = event.target.files || event.dataTransfer.files;

		// process all File objects
		for (let i = 0, file; file = files[i]; i++)
		{
			upload_file(file);
		}
	}, false);

	document.addEventListener("dragenter", ev=>{ev.preventDefault();}, false);
	document.addEventListener("dragover",  ev=>{ev.preventDefault();}, false);
}

function show_image(blob_id)
{
	show_lightbox("/blob/" + blob_id);
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
		console.log("delete", document.getElementById("light-box").dataset.blob_id);

		let req = new XMLHttpRequest();
		req.open("DELETE", "/blob/" + document.getElementById("light-box").dataset.blob_id);
		req.onload = () =>
		{
			// extract blob_id from the location
			if (req.status === 200)
			{
				console.log("deleted");
			}
			else
			{
				console.warn("Upload failure:", req.status);
			}
		};
		req.send(null);
	}
}
