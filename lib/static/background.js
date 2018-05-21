/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

let action_queue = [];
let current_request = null;


function queue_action(action)
{
	console.log("queuing action " + action);

	action_queue.push(action);
	start_queued_actions();
}

function start_queued_actions()
{
	if (current_request == null && action_queue.length > 0)
	{
		let action = action_queue.shift();
		if (action.upload)
			upload_file(action.upload);

		else if (action.move && action.dest)
			move_image(action.move, action.dest);
	}

	else if (current_request == null && action_queue.length === 0)
	{
		hide_toast("Finished all background tasks");
	}
}

function move_image(blobid, dest)
{
	show_toast("Moving " + blobid + " to " + dest);
	post_blob(blobid, "move="+dest, () =>
	{
		// display the image in its new collection
		close_overlays(false);
		change_dir(dest, ()=>{show_image(blobid, true);});
		window.last_moved_album = dest;

		// notify user
		hide_toast("Image moved successfully");
	});
}

function upload_file(file)
{
	show_toast("uploading " + file.name +
		(action_queue.length > 0 ? (" (" + action_queue.length + " pending)") : "")
	);

	current_request = new XMLHttpRequest();
	current_request.open("PUT", path_url("upload", file.name));
	current_request.onload = () =>
	{
		// extract blob_id from the location
		if (current_request.status === 201)
		{
			const blob_id = current_request.getResponseHeader("location").substr(-40);
			dir.elements[blob_id] = {mime: file.type, filename: file.name, perm: "private"};

			show_image(blob_id, true);
			add_element(blob_id);

			// automatically change permission
			// TODO: use action queue to change permission
//			if (dir.elements[blob_id].perm !== default_perm)
//				set_blob_permission(blob_id, default_perm);
		}
		else
		{
			console.warn("Upload failure:", req.status);
			alert("onload(): cannot upload file: " + req.statusText);
			close_overlays(false);
		}

		current_request = null;

		// start next queued upload
		console.log("upload complete. starting next upload");
		start_queued_actions();
	};
	current_request.onerror = () =>
	{
		alert("onerror(): cannot upload file: " + req.statusText);
		close_overlays(false);
	};
	current_request.send(file);
}

function post_blob(blobid, data, onsuccess)
{
	current_request = new XMLHttpRequest();
	current_request.open("POST", path_url("api", blobid));
	current_request.onload = () =>
	{
		if (current_request.status === 204)
		{
			onsuccess(current_request);
			start_queued_actions();
		}
		else
		{
			console.log("cannot post blob", current_request.status, current_request.responseText);
		}
	};
	current_request.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
	current_request.send(data);
}
