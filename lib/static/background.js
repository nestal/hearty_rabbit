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
	console.log("queuing action " + JSON.stringify(action));

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

		else if (current_action.perm && current_action.blob)
			return set_blob_permission(current_action.blob, current_action.perm).then(() =>
			{
				current_action = null;
				start_queued_actions();
			});
	}

	else if (current_action == null && action_queue.length === 0)
	{
		console.log("Finished all background tasks -> Reloading current directory");
		hide_toast("Finished all background tasks");

		close_overlays(false);
		change_dir(current_path_intent());
	}
}

function move_image(path_intent, dest)
{
	show_toast("Moving " + path_intent.filename + " to " + dest);
	post_path(path_intent, "move="+dest).then(
		() =>
		{
			current_action = null;
			last_moved_album = dest;

			// notify user
			console.log("Successfully moved " + path_intent.filename + " to " + dest);
			hide_toast("Image moved successfully");

			start_queued_actions();
		},
		(err) =>
		{
			console.error("cannot move " + blobid + " to " + dest + ": " + err);
			current_action = null;
			start_queued_actions();
		}
	);
}

function upload_file(file)
{
	show_toast("uploading " + file.name +
		(action_queue.length > 0 ? (" (" + action_queue.length + " pending)") : "")
	);

	let intent = Intent.current();
	intent.filename = file.name;
	intent.action = "upload";

	console.log("uploading to ", intent.url());

	let req = new XMLHttpRequest();
	req.open("PUT", intent.url());
	req.onload = () =>
	{
		// no matter whether success or not, the current action is finished
		current_action = null;

		console.log("uploaded", req);

		// extract blob_id from the location
		if (req.status === 201)
		{
			const blob_id = req.getResponseHeader("location").substr(-40);
			dir.elements[blob_id] = {mime: file.type, filename: file.name, perm: "private"};

			add_element(blob_id);

			// automatically change permission to default_perm (i.e. last permission set by user)
			if (dir.elements[blob_id].perm !== default_perm)
				set_blob_permission(blob_id, default_perm);
		}
		else
		{
			console.warn("Upload failure:", req.status);
			alert("onload(): cannot upload file: " + req.statusText);
//			close_overlays(false);
		}

		// start next queued upload
		console.log("upload complete. starting next upload");
//		start_queued_actions();
	};
	req.onerror = () =>
	{
		current_action = null;

		alert("onerror(): cannot upload file: " + req.statusText);
//		close_overlays(false);

//		start_queued_actions();
	};
	req.send(file);
}

function post_blob(blobid, data)
{
	return post_path(get_blob_intent(blobid), data);
}

function post_path(path_intent, data)
{
	return new Promise((resolve, reject) =>
	{
		let req = new XMLHttpRequest();
		const purl = path_intent.api().url();
		console.log("posting to " + purl);

		req.open("POST", purl);
		req.onload = () =>
		{
			if (req.status === 204)
			{
				resolve(req);
			}
			else
			{
				console.log("cannot post blob", req.status, req.responseText);
				reject(req.status);
			}
		};
		req.onerror = () =>
		{
			console.error("cannot post blob: onerror(): " + event.toString());
			reject(req.status);
		};
		req.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
		req.send(data);
	});
}
