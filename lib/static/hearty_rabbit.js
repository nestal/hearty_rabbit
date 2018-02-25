/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

function on_hrb_drop(ev)
{
	console.log("dropping: " + event.target.className);
	console.log("on drop!!");
	ev.preventDefault();
/*	var data = ev.dataTransfer.getData("text");

	var files = ev.target.files || ev.dataTransfer.files;

	// process all File objects
	for (var i = 0, file; file = files[i]; i++)
		upload_file(file);*/
}

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

	root.removeChild(proto);

	document.addEventListener("drop", function( event )
	{
		event.preventDefault();
		console.log("dropped!", event.target.className);

		var data = event.dataTransfer.getData("text");

		var files = event.target.files || event.dataTransfer.files;

		// process all File objects
		for (var i = 0, file; file = files[i]; i++)
			upload_file(file);
	}, false);

	document.addEventListener("dragenter", function( event )
	{
		event.preventDefault();

		// highlight potential drop target when the draggable element enters it
		console.log("drageneter", event.target.className);
	}, false);
	document.addEventListener("dragover", function( event )
	{
		event.preventDefault();

		// highlight potential drop target when the draggable element enters it
		console.log("dragover", event.target.className);
	}, false);
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
