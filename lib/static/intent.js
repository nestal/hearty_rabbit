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

	static current()
	{
		return Intent.from_url(new URL(document.URL));
	}

	static from_url(url)
	{
		let path = url.pathname.split('/');
		if (path[0] === '')
			path.shift();

		let r = new Intent();
		r.query  = url.searchParams;

		if (path.length >= 2)
		{
			r.action = path.shift();
			r.owner  = path.shift();

			// may need to look at the action to see if all filenames are 40 bytes
			// some actions does not have filenames
			if ((r.action !== "view" && r.action !== "api") || (path.length > 0 && path[path.length-1].length === 40))
				r.filename = path.pop();

			r.coll = path.join('/');
		}
		return r;
	}

	static collection(owner, coll)
	{
		let r = new Intent();
		r.owner = owner;
		r.coll = coll;
		return r;
	}

	static make(action, owner, coll, filename, query)
	{
		let r = new Intent();
		r.action = action;
		r.owner = owner;
		r.coll = coll;
		r.filename = filename;
		r.query = query;
		return r;
	}

	clone()
	{
		return Intent.make(this.action, this.owner, this.coll, this.filename, this.query);
	}

	view()
	{
		return Intent.make("view", this.owner, this.coll, this.filename, this.query);
	}

	api()
	{
		return Intent.make("api", this.owner, this.coll, this.filename, this.query);
	}

	parent_collection()
	{
		return Intent.make(this.action, this.owner, this.coll, null, null);
	}

	url()
	{
		let owner = this.owner;
		if (owner == null)
			owner = "";

		let collection = this.coll;
		if (collection == null)
			collection = "";

		let query = this.query;
		if (query == null)
			query = new URLSearchParams();
		if (dir.auth != null)
			query.set("auth", dir.auth);

		let filename = this.filename;
		if (filename == null)
			filename = "";

		return "/" + (owner.length === 0 && collection.length === 0 && filename.length === 0 ? "" : this.action) +
			(owner.length      > 0 ? "/" : "") + owner +
			(collection.length > 0 ? "/" : "") + collection +
			(filename.length   > 0 ? "/" : "") + filename +
			(query.toString().length > 0 ? "?" : "") + query.toString();
	}
}