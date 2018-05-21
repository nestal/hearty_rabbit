/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

let action_queue = [];
let current_action = null;

function queue_action(action)
{
	console.log("queuing action " + action);

	action_queue.push(action);
	start_queued_actions();
}

function start_queued_actions()
{
	if (current_action == null && action_queue.length > 0)
	{
		current_action = action_queue.shift();
		if (current_action.upload)
			upload_file(current_action.upload);

		else if (current_action.move && current_action.dest)
			move_image(current_action.move, current_action.dest);
	}

	else if (current_action == null && action_queue.length === 0)
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

	let req = new XMLHttpRequest();
	req.open("PUT", path_url("upload", file.name));
	req.onload = () =>
	{
		// no matter whether success or not, the current action is finished
		current_action = null;

		// extract blob_id from the location
		if (req.status === 201)
		{
			const blob_id = req.getResponseHeader("location").substr(-40);
			dir.elements[blob_id] = {mime: file.type, filename: file.name, perm: "private"};

			show_image(blob_id, true);
			add_element(blob_id);

			// automatically change permission
			// TODO: use action queue to change permission
			if (dir.elements[blob_id].perm !== default_perm)
				set_blob_permission(blob_id, default_perm);
		}
		else
		{
			console.warn("Upload failure:", req.status);
			alert("onload(): cannot upload file: " + req.statusText);
			close_overlays(false);
		}

		// start next queued upload
		console.log("upload complete. starting next upload");
		start_queued_actions();
	};
	req.onerror = () =>
	{
		current_action = null;

		alert("onerror(): cannot upload file: " + req.statusText);
		close_overlays(false);

		start_queued_actions();
	};
	req.send(file);
}

function post_blob(blobid, data, onsuccess)
{
	let req = new XMLHttpRequest();
	req.open("POST", path_url("api", blobid));
	req.onload = () =>
	{
		current_action = null;
		if (req.status === 204)
		{
			onsuccess(req);
		}
		else
		{
			console.log("cannot post blob", req.status, req.responseText);
		}
		start_queued_actions();
	};
	req.onerror = () =>
	{
		current_action = null;
		start_queued_actions();
	};
	req.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
	req.send(data);
}
