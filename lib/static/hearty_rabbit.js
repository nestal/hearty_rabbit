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
    req.onload = function(event)
    {
        console.log(req.getResponseHeader("location"));
        window.location = req.getResponseHeader("location");
    }
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
        console.log("here is a blob", element);
        const entry = proto.cloneNode(true);
        entry.id = "hrb-entry-" + element;
        entry.src = "/blob/" + element;
        root.insertBefore(entry, proto);
    });

    root.removeChild(proto);
}
