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

	const entry = proto.cloneNode(true);
	entry.dataset.blob_id = blob_id;
	entry.id = "hrb-entry-" + blob_id;

	// Assign blob URL to images
	entry.querySelector("img").src = "/blob/" + blob_id;

	// check orientation
/*	const orientation = dir.elements[blob_id].orientation;
	if (orientation !== undefined && orientation !== 1)
	{
		console.log("image", blob_id, "need rotation", dir.elements[blob_id].orientation);
		img.classList.add("rotate270");
		img.width = img.height;
	}*/

	root.insertBefore(entry, proto);
}

function upload_file(file)
{
	let req = new XMLHttpRequest();
	req.open("PUT", "/upload/" + file.name);
	req.onload = () =>
	{
		// extract blob_id from the location
		const re = /\/blob\/([a-f0-9]{40})/.exec(req.getResponseHeader("location"));
		if (re && req.status === 201)
		{
			const blob_id = re[1];
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
			upload_file(file);
	}, false);

	document.addEventListener("dragenter", ev=>{ev.preventDefault();}, false);
	document.addEventListener("dragover",  ev=>{ev.preventDefault();}, false);
}

function show_image(blob_id)
{
	show_lightbox("/blob/" + blob_id);
}

function show_lightbox(url)
{
	// unhide the lightbox and set its src to the blob URL
	document.getElementById("light-box-image").src = url;
	document.getElementById("light-box").style.display = "block";
}

function hide_image(img)
{
	document.getElementById("light-box").style.display = "none";
}
