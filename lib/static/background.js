/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

function move_image(path_intent, dest)
{
	show_toast("Moving " + path_intent.filename + " to " + dest);
	post_path(path_intent, "move="+dest).then(
		() =>
		{
			last_moved_album = dest;

			// notify user
			console.log("Successfully moved " + path_intent.filename + " to " + dest);
			hide_toast("Image moved successfully");
		},
		(err) =>
		{
			console.error("cannot move " + blobid + " to " + dest + ": " + err);
		}
	);
}

function upload_file(file)
{
	show_toast("uploading " + file.name);

	let intent = Intent.current();
	intent.filename = file.name;
	intent.action = "upload";

	console.log("uploading to ", intent.url());

	let req = new XMLHttpRequest();
	req.open("PUT", intent.url());
	req.onload = () =>
	{
		console.log("uploaded", req);
		if (req.status === 201)
		{
			// extract blob_id from the location
			const blob_id = req.getResponseHeader("location").substr(-40);

			if (dir.elements[blob_id] == null)
			{
				dir.elements[blob_id] = {mime: file.type, filename: file.name, perm: "private"};
				add_element(blob_id);

				// automatically change permission to default_perm (i.e. last permission set by user)
				if (dir.elements[blob_id].perm !== default_perm)
					set_blob_permission(blob_id, default_perm);
			}
			else
			{
				console.warn(blob_id, "already added");
			}
		}
		else
		{
			console.warn("Upload failure:", req.status);
			alert("onload(): cannot upload file: " + req.statusText);
		}

		// start next queued upload
		console.log(blob_id, "upload complete.");
	};
	req.onerror = () =>
	{
		alert("onerror(): cannot upload file: " + req.statusText);
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
