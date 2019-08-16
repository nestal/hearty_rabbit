// Encapsulation of what the user wants to do
class Intent
{
	constructor()
	{
		this.action = "view";
		this.owner  = null;
		this.coll   = null;
		this.filename = null;
		this.query  = null;
	}

	static from_url(url)
	{
		let path = url.pathname.split('/');

		let r = new Intent();
		r.action = path[1];
		r.owner  = path[2];
		r.coll   = path[3];
		r.filename = path[4];
		r.query  = url.searchParams;

		console.log(r);

		return r;
	}

	static make(action, coll, filename, rendition)
	{
		let r = new Intent();
		r.action = action;
		r.coll = coll;
		r.filename = filename;
		r.query = new URLSearchParams();
		r.query.set("rendition", rendition);
		return r;
	}

	url()
	{
		if (this.coll != null)
		{
			let owner = this.owner;
			if (owner == null)
				return "/";

			let collection = this.coll;
			let query = this.query;
			if (query == null && dir.auth != null)
				query = new URLSearchParams();
			if (dir.auth != null)
				query.set("auth", dir.auth);

			let filename = this.filename;
			if (filename == null)
				filename = "";

			return "/" + this.action + "/" + owner + (collection.length > 0 ? "/" : "") + collection + "/" + filename;
		}
	}
}