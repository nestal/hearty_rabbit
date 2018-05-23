/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

// Called after the selection is changed to reflect the selection status on screen
function update_selected_blobs()
{
	// the nav button that appear only when the user selects something
	let nav = document.querySelectorAll(".topnav .show-only-when-selected");

	const selected_count = get_selected_blobs().length;
	if (selected_count > 0)
	{
		for (let i = 0; i < nav.length; i++)
			nav[i].style.display = "block";
		show_toast(selected_count + " images(s) selected");
	}
	else
	{
		for (let i = 0; i < nav.length; i++)
			nav[i].style.display = "none";
		hide_toast();
	}
}

function is_blob_selected(blob_id)
{
	return (dir.elements[blob_id].selected == null) ? false : dir.elements[blob_id].selected;
}

function select_blob(blob_id, select)
{
	if (select == null)
		select = true;

	if (dir.elements[blob_id] != null)
	{
		dir.elements[blob_id].selected = select;

		let entry = document.getElementById("hrb-entry-" + blob_id);
		if (entry != null)
		{
			if (select)
				entry.classList.add("selected-entry");
			else
				entry.classList.remove("selected-entry");
		}

		update_permission_icons(blob_id);
		update_selected_blobs();
	}
}

// Select or de-select a blob
function toggle_select_blob(blob_id)
{
	select_blob(blob_id, !is_blob_selected(blob_id));
}

// Return an array of selected blob IDs
function get_selected_blobs()
{
	let selection = [];
	if (dir.elements != null)
	{
		Object.keys(dir.elements).forEach(blob_id =>
		{
			if (dir.elements[blob_id].selected != null && dir.elements[blob_id].selected === true)
				selection.push(blob_id);
		});
	}
	return selection;
}

function select_all_blobs(select)
{
	// default is select all. pass "false" to deselect all
	if (select == null)
		select = true;

	if (dir.elements != null)
	{
		Object.keys(dir.elements).forEach(blob_id =>
		{
			select_blob(blob_id, select);
		});
	}

	update_selected_blobs();
}
