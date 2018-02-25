/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

function upload_file(file)
{
	var req = new XMLHttpRequest();
	req.open("PUT", "/upload/" + file.name);
	req.onload = function (event)
	{
		document.getElementById("light-box-image").src = req.getResponseHeader("location");
		document.getElementById("light-box").style.display = "block";
	};
	req.send(file);
}

function on_home_page_loaded()
{
	document.getElementById("username").textContent = dir.name;

	// create one new entry for each blob
	const proto = document.getElementById("hrb-entry-prototype");
	const root = document.getElementById("hrb-entry-root");

	dir.elements.forEach(element =>
	{
		const entry = proto.cloneNode(true);
		entry.dataset.blob_id = element;
		entry.id = "hrb-entry-" + element;
		entry.src = "/blob/" + element;
		root.insertBefore(entry, proto);
	});

//	root.removeChild(proto);

	document.addEventListener("drop", function( event )
	{
		event.preventDefault();
		var files = event.target.files || event.dataTransfer.files;

		// process all File objects
		for (var i = 0, file; file = files[i]; i++)
			upload_file(file);
	}, false);

	document.addEventListener("dragenter", ev=>{ev.preventDefault();}, false);
	document.addEventListener("dragover",  ev=>{ev.preventDefault();}, false);
}

function show_image(blob_id)
{
	console.log("showimage(): ", blob_id);
	document.getElementById("light-box-image").src = "/blob/" + blob_id;
	document.getElementById("light-box").style.display = "block";
}

function hide_image(img)
{
	document.getElementById("light-box").style.display = "none";
}
