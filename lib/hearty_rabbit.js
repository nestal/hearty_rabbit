/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

function upload_file(files)
{
    var req = new XMLHttpRequest();
    req.open("PUT", "/upload");
    req.onload = function(event) { console.log(event.target.responseText); }
    req.send(files[0]);
}
