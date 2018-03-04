/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

function add_blob(element)
{
	console.log("adding blob", element);

	// TODO: show non-image blobs
	if (element.mime !== "image/jpeg")
		return;

	// create one new entry for each blob
	const proto = document.getElementById("hrb-entry-prototype");
	const root = document.getElementById("hrb-entry-root");

	// Clone the prototype node and update its ID
	const entry = proto.cloneNode(true);
	entry.id = "hrb-entry-" + element.blob_id;

	// Assign blob URL to the image in the cloned node
	const img = entry.querySelector("img");
	img.src = "/blob/" + element.blob_id;
	img.dataset.blob_id = element.blob_id;

	// Show the cloned node
	root.insertBefore(entry, proto);
}

function upload_file(file)
{
	console.log("uploading", file.name, file.type, "file");

	let req = new XMLHttpRequest();
	req.open("PUT", "/upload" + dir.path + "/" + file.name);
	req.onload = () =>
	{
		// extract blob_id from the location
		const re = /\/blob\/([a-f0-9]{40})/.exec(req.getResponseHeader("location"));
		if (re && req.status === 201)
		{
			const blob_id = re[1];
			const element = {mime: file.type, filename: file.name, blob_id: blob_id};
			dir.elements.push(element);

			console.log("uploaded blob", element);
			console.log("blobs: ", dir.elements);

			show_image(blob_id);
			add_blob(element);
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

	console.log("dir.elements", dir.elements);

	dir.elements.forEach(element =>
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

function index_of_image(blob_id)
{
	return dir.elements.findIndex(element => {return element.blob_id === blob_id;});
}

function delete_image()
{
	if (confirm("Do you want to delete this image"))
	{
		const blob_id = document.getElementById("light-box").dataset.blob_id;
		console.log("deleting blob", blob_id);

		let req = new XMLHttpRequest();
		req.open("DELETE", "/blob/" + blob_id);
		req.onload = () =>
		{
			// extract blob_id from the location
			if (req.status === 202)
			{
				console.log("deleted blob", blob_id, "successfully");
				const index = dir.elements.find(element => {return element.blob_id === blob_id;});
				dir.elements.splice(index,1);
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

function next_image(offset)
{
	const sign = (offset > 0 ? 1 : -1);

	const current = document.getElementById("light-box").dataset.blob_id;
	if (current)
	{
		const index = index_of_image(current);
		console.log("current index", index, dir.elements.length);
		for (let i = 0 ; i < dir.elements.length; ++i)
		{
			const next_index = (index + offset + sign*i + dir.elements.length) % dir.elements.length;
			console.log("next index", next_index, dir.elements.length);
			const next = dir.elements[next_index];
			if (next.mime === "image/jpeg")
			{
				show_image(next.blob_id);
				break;
			}
		}
	}
}
